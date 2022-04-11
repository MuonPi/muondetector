#ifndef MUONPI_CONFIG_H
#define MUONPI_CONFIG_H

#include "version.h"

#include <chrono>
#include <cstdint>
#include <string>

namespace MuonPi::Version {
constexpr struct Version {
    int major;
    int minor;
    int patch;
    const char* additional { "" };
    const char* hash { "" };

    [[nodiscard]] auto string() const -> std::string;
} hardware { 3, 0, 0 },
    software { CMake::Version::major, CMake::Version::minor, CMake::Version::patch, CMake::Version::additional, CMake::Version::hash };
}

namespace MuonPi::Config {
constexpr const char* file { "/etc/muondetector/muondetector.conf" };
constexpr const char* data_path { "/var/muondetector/" };
constexpr int event_count_deadtime_ticks { 1000 };
constexpr int event_count_max_pileups { 50 };

namespace MQTT {
    constexpr const char* host { "data.muonpi.org" };
    constexpr int port { 1883 };
    constexpr int timeout { 10000 };
    constexpr int qos { 1 };
    constexpr int keepalive_interval { 45 };
    constexpr const char* data_topic { "muonpi/data/" };
    constexpr const char* log_topic { "muonpi/log/" };
}
namespace Log {
    constexpr std::chrono::seconds interval { 60 };
    constexpr int max_geohash_length { 6 };
    constexpr std::chrono::seconds rotate_period_default { 7 * 86400UL };
}
namespace Hardware {
    namespace OLED {
        constexpr int update_interval { 2000 };
    }
    namespace ADC {
        constexpr int buffer_size { 50 };
        constexpr int pretrigger { 10 };
        constexpr int deadtime { 8 };
    }
    namespace DAC {
        namespace Channel {
            constexpr int bias { 2 }; //!< channel of the dac where bias voltage is set
            constexpr int threshold[2] { 0, 1 }; //!< channel of the dac where thresholds 1 and 2 are set
        }
        namespace Voltage {
            constexpr float bias { 0.5 };
            constexpr float threshold[2] { 0.1, 1.0 };
        }
    }
    namespace GPIO::Clock::Measurement {
        constexpr int interval { 100 };
        constexpr int buffer_size { 500 };
    }
    constexpr int trace_sampling_interval { 5 };
    constexpr int monitor_interval { 5000 };
    namespace RateScan {
        constexpr int iterations { 10 };
        constexpr int interval { 400 };
    }
}

} // namespace MuonPi::Config

namespace MuonPi::Settings {
struct {
    int max_geohash_length { Config::Log::max_geohash_length };
    std::chrono::seconds rotate_period { Config::Log::rotate_period_default };
} log;

struct {
    bool store_local { false };
} events;

struct {
    int port { 51508 };
} gui;
}

#endif // MUONPI_CONFIG_H
