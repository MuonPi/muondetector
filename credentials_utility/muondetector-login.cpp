#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/thread_pool.h"
#include "data/events/mqtt_status_event.h"
#include "sinks/mqtt_sink.h"

#include <condition_variable>
#include <config.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <grp.h>
#include <iostream>
#include <mutex>
#include <regex>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

[[nodiscard]] auto getch() -> char;
[[nodiscard]] auto getpasswd(const char* prompt, const bool show_asterisk) -> std::string;

namespace {
constexpr const char* config_env{"MUONDETECTOR_CONFIG_FILE"};

auto configPath() -> std::filesystem::path {
    if (const char* path = std::getenv(config_env); path != nullptr && path[0] != '\0') {
        return path;
    }
    return MuonPi::Config::file;
}

auto trimLeft(const std::string& value) -> std::string {
    const auto start = value.find_first_not_of(" \t");
    if (start == std::string::npos) {
        return {};
    }
    return value.substr(start);
}

auto configEscape(const std::string& value) -> std::string {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
        }
    }
    return escaped;
}

auto configUnescape(const std::string& value) -> std::string {
    std::string unescaped;
    bool escaped = false;
    for (const char ch : value) {
        if (escaped) {
            switch (ch) {
                case 'n':
                    unescaped += '\n';
                    break;
                case 'r':
                    unescaped += '\r';
                    break;
                case 't':
                    unescaped += '\t';
                    break;
                default:
                    unescaped += ch;
                    break;
            }
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else {
            unescaped += ch;
        }
    }
    return unescaped;
}

auto parseConfigString(const std::vector<std::string>& lines,
                       const std::string& key) -> std::string {
    const std::regex assignment{"^\\s*" + key +
                                "\\s*=\\s*\"((?:\\\\.|[^\"])*)\"\\s*;?\\s*(?:#.*)?$"};
    std::smatch match;
    for (const auto& line : lines) {
        if (std::regex_match(line, match, assignment)) {
            return configUnescape(match[1].str());
        }
    }
    return {};
}

auto readConfigLines(const std::filesystem::path& path) -> std::vector<std::string> {
    std::ifstream input{path};
    if (!input) {
        throw std::runtime_error{"Could not open config file: " + path.string()};
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) {
        lines.push_back(line);
    }
    return lines;
}

auto isCredentialAssignment(const std::string& line) -> bool {
    const auto trimmed = trimLeft(line);
    return trimmed.starts_with("mqtt_user") || trimmed.starts_with("mqtt_password") ||
           trimmed == "# MQTT credentials written by muondetector-login.";
}

auto writeConfigLines(const std::filesystem::path& path,
                      const std::vector<std::string>& lines) -> bool {
    const auto tmpPath = path.string() + ".tmp";
    {
        std::ofstream output{tmpPath, std::ios::trunc};
        if (!output) {
            logError("Could not open temporary config file for writing: " + tmpPath);
            return false;
        }
        for (const auto& line : lines) {
            output << line << '\n';
        }
    }

    std::error_code error;
    std::filesystem::rename(tmpPath, path, error);
    if (error) {
        logError("Could not replace config file '" + path.string() + "': " + error.message());
        std::filesystem::remove(tmpPath);
        return false;
    }

    chmod(path.c_str(), S_IRUSR | S_IWUSR | S_IRGRP);
    if (auto* group = getgrnam("users"); group != nullptr) {
        chown(path.c_str(), 0, group->gr_gid);
    }
    return true;
}

auto storeCredentialsInConfig(const std::string& username, const std::string& password) -> bool {
    const auto path = configPath();
    std::vector<std::string> lines;
    try {
        lines = readConfigLines(path);
    } catch (const std::exception& error) {
        logError(error.what());
        return false;
    }

    std::vector<std::string> updated;
    updated.reserve(lines.size() + 3);
    for (const auto& line : lines) {
        if (!isCredentialAssignment(line)) {
            updated.push_back(line);
        }
    }

    updated.push_back("");
    updated.push_back("# MQTT credentials written by muondetector-login.");
    updated.push_back("mqtt_user = \"" + configEscape(username) + "\"");
    updated.push_back("mqtt_password = \"" + configEscape(password) + "\"");

    return writeConfigLines(path, updated);
}

auto clearCredentialsFromConfig() -> bool {
    const auto path = configPath();
    std::vector<std::string> lines;
    try {
        lines = readConfigLines(path);
    } catch (const std::exception& error) {
        logError(error.what());
        return false;
    }

    std::vector<std::string> updated;
    updated.reserve(lines.size());
    for (const auto& line : lines) {
        if (!isCredentialAssignment(line)) {
            updated.push_back(line);
        }
    }

    return writeConfigLines(path, updated);
}
} // namespace

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
    if (!storeCredentialsInConfig(username, password)) {
        return -1;
    }
    logInfo("Stored MQTT credentials in " + configPath().string());
    return 0;
}

void printUsage(const char* program) {
    std::cout << "Usage:\n"
              << "  " << program << " store\n"
              << "  " << program << " get\n"
              << "  " << program << " clear\n";
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
        std::vector<std::string> lines;
        try {
            lines = readConfigLines(configPath());
        } catch (const std::exception& error) {
            logError(error.what());
            return 1;
        }
        const auto username = parseConfigString(lines, "mqtt_user");
        const auto password = parseConfigString(lines, "mqtt_password");
        if (username.empty() || password.empty()) {
            logWarn("No credentials found");
            return 2;
        }

        std::cout << "Credentials in " << configPath() << ":\n"
                  << "username: " << username << "\n"
                  << "password: [hidden]\n";
    } else if (command == "clear") {

        if (!clearCredentialsFromConfig()) {
            return 1;
        }

        std::cout << "Credentials cleared\n";
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
