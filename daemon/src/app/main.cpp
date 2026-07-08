
#include "config_parser.h"
#include "core/event_bus.h"
#include "core/scheduler.h"
#include "core/system_builder.h"
#include "core/thread_pool.h"
#include "network/networkdiscovery.h"
#include "system_config.h"

#include <cstdio>
#include <iostream>
#include <libconfig.h++>
// #include <termios.h>
// #include <unistd.h>
#include <atomic>
#include <config.h>
#include <csignal>
#include <gpio_pin_definitions.h>
#include <pthread.h>

static const std::string CONFIG_FILE = std::string(MuonPi::Config::file);
static const std::string SETTINGS_FILE =
    std::string(MuonPi::Config::data_path) + std::string(MuonPi::Config::persistant_settings_file);

namespace Runtime {
inline std::atomic<bool> g_running = true;
}

extern "C" void handleSignal(int) {
    Runtime::g_running = false;
}

int main(int argc, char* argv[]) {
    // first, we must set the locale to be independent of the number format of the system's locale.
    // We rely on parsing floating point numbers with a decimal point (not a komma) which might fail
    // if not setting the classic locale
    std::locale::global(std::locale::classic());

    // QCoreApplication a(argc, argv);
    // QCoreApplication::setApplicationName("muondetector-daemon");
    // QCoreApplication::setApplicationVersion(std::string::fromStdString(MuonPi::Version::software.string()));
    // config file handling

    std::cout << "MuonPi Muondetector Daemon "
              << "V" + MuonPi::Version::software.string()
              << "(build " + std::string(__TIMESTAMP__) + ")\n";

    // Read the file. If there is an error, report it and exit.
    auto cfg = ConfigParser::loadConfigFile(CONFIG_FILE);

    // Read in the settings file. If there is an error, create the settings
    // tree and proceed anyway

    auto settings = std::make_shared<libconfig::Config>();
    try {
        settings = ConfigParser::loadConfigFile(SETTINGS_FILE);
    } catch (const libconfig::FileIOException& fioex) {
        // Find the stored settings in settings file. Add all entries if they don't yet
        // exist.
        libconfig::Setting& root = settings->getRoot();
        // create setting fields
        root.add("geo_handling", libconfig::Setting::TypeGroup);
        libconfig::Setting& geo_handling = root["geo_handling"];
        geo_handling.add("mode", libconfig::Setting::TypeString) = "Auto";
        geo_handling.add("static_coordinates", libconfig::Setting::TypeGroup);
        libconfig::Setting& static_coords = geo_handling["static_coordinates"];
        static_coords.add("lon", libconfig::Setting::TypeFloat) = 0.;
        static_coords.add("lat", libconfig::Setting::TypeFloat) = 0.;
        static_coords.add("alt", libconfig::Setting::TypeFloat) = 0.;
        static_coords.add("hor_error", libconfig::Setting::TypeFloat) = 0.;
        static_coords.add("vert_error", libconfig::Setting::TypeFloat) = 0.;
        // Write out the updated configuration.
        try {
            settings->writeFile(SETTINGS_FILE.c_str());
            std::cout << "Initialized settings successfully written to: " << SETTINGS_FILE
                      << std::endl;
        } catch (const libconfig::FileIOException& fioex_new) {
            std::cerr << "I/O error while writing settings file: " << SETTINGS_FILE << std::endl;
        }
    } catch (const libconfig::ParseException& pex) {
        std::cerr << "Parse error at " + std::string(pex.getFile()) + " : line " +
                         std::to_string(pex.getLine()) + " - " + std::string(pex.getError())
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Find the stored settings in settings file. Add intermediate entries if they don't yet
    // exist.
    libconfig::Setting& root = settings->getRoot();

    if (root.exists("geo_handling") == false) {
        std::cerr << "error accessing settings. Aborting...";
        return EXIT_FAILURE;
    }

    auto config = SystemConfig{};

    config.config_file_data = cfg;
    config.settings_file_data = settings;
    config = ConfigParser(argc, argv, std::move(config))
                 .get(); // Loads default settings and updates them with commandline arguments

    std::signal(SIGTERM, handleSignal);
    std::signal(SIGINT, handleSignal);
    // START

    ThreadPool pool{[&]() -> std::size_t {
        std::size_t n = 0;

        n = config.max_thread_count;

        if (n == 0) {
            n = std::thread::hardware_concurrency();
        }

        if (n == 0) {
            n = 1; // absolute fallback
        }

        n = std::min<std::size_t>(n, 64);

        return n;
    }()};

    auto context = SystemBuilder::build(pool, config);
    auto work_guard = boost::asio::make_work_guard(context.io);
    std::thread t1([&] {
        pthread_setname_np(pthread_self(), "muondet-asio");
        context.io->run();
    });

    context.scheduler->start();

    auto networkDiscovery =
        std::make_unique<NetworkDiscovery>(NetworkDiscovery::DeviceType::DAEMON);
    networkDiscovery->start();

    while (Runtime::g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    networkDiscovery.reset();
    context.scheduler->stop();
    context.sinks->shutdownAll();
    pool.stop();

    // stop asio
    work_guard.reset();
    context.io->stop();

    // wait for io thread
    t1.join();

    return EXIT_SUCCESS;
}
