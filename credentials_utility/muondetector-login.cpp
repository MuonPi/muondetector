#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/thread_pool.h"
#include "data/events/mqtt_status_event.h"
#include "libsecret/credentials.h"
#include "sinks/mqtt_sink.h"

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <string>
#include <termios.h>
#include <thread>
#include <unistd.h>

[[nodiscard]] auto getch() -> char;
[[nodiscard]] auto getpasswd(const char* prompt, const bool show_asterisk) -> std::string;

auto getch() -> char {
    termios t_old{};
    termios t_new{};

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO));
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    char ch{static_cast<char>(getchar())};

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

auto getpasswd(const char* prompt, const bool show_asterisk) -> std::string {
    const char BACKSPACE = 127;
    const char RETURN = 10;

    std::string password{};
    char ch{getch()};
    std::cout << prompt;
    while ((ch = getch()) != RETURN) {
        if (ch == BACKSPACE) {
            if (password.length() != 0) {
                if (show_asterisk)
                    std::cout << "\b \b";
                password.resize(password.length() - 1);
            }
        } else {
            password += ch;
            if (show_asterisk) {
                std::cout << '*';
            }
        }
    }
    std::cout << std::endl;
    return password;
}

auto doStore() -> int {
    std::cout << "To set the login for the mqtt-server, please enter your credentials.\nusername: ";
    std::string username{};
    std::cin >> username;
    std::string password{getpasswd("password: ", true)};

    ThreadPool pool;
    EventBus bus{pool};
    MqttSink mqtt{bus, ""};
    std::condition_variable wakeup{};
    std::atomic<bool> connected{false};
    bus.subscribe<MqttStatusEvent>([&](const MqttStatusEvent& event) {
        switch (event.status) {
            case MqttStatusEvent::Status::Connected:
                connected = true;
                wakeup.notify_all();
                break;
            case MqttStatusEvent::Status::Error:
                wakeup.notify_all();
                break;
            default:
                logDebug("Status: " + std::to_string(static_cast<unsigned>(event.status)));
                break;
        }
        if (event.text.empty() == false) {
            logInfo(event.text);
        }
    });
    mqtt.start(username, password);

    std::mutex mx;
    std::unique_lock<std::mutex> lock{mx};
    auto result = wakeup.wait_for(lock, std::chrono::seconds(3));
    if (result == std::cv_status::timeout) {
        logError("Timeout connecting to MQTT server");
    }
    pool.stop();
    if (!connected) {
        return -1;
    }
    Credentials::storeCredentials(username, password);
    return 0;
}

void printUsage(const char* program) {
    std::cout << "Usage:\n"
              << "  " << program << " store <username> <password>\n"
              << "  " << program << " get <username>\n";
}

int main(int argc, char** argv) {
    setLogLevel(LogLevel::Info);
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string command = argv[1];

    if (command == "store") {
        return doStore();
    } else if (command == "get") {

        auto credentials = Credentials::retrieveCredentials();

        if (credentials.empty()) {
            logWarn("No credentials found");
            return 2;
        }

        std::cout << "Credentials: " << credentials << "\n";
    } else if (command == "clear") {

        Credentials::clearCredentials();

        std::cout << "Credentials cleared\n";
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
