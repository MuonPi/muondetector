#ifndef MUONPI_CONFIG_H
#define MUONPI_CONFIG_H

#include "version.h"

#include <cstdint>
#include <string>


namespace MuonPi::Version {
constexpr struct Version {
    int major;
    int minor;
    int patch;

    [[nodiscard]] auto string() const -> std::string;
}
    hardware { 2, 0, 0 },
    software { CMake::Version::major, CMake::Version::minor, CMake::Version::patch };
}

namespace MuonPi::Config {
constexpr const char* file { "/etc/muondetector/muondetector.conf" };
constexpr int event_count_deadtime_ticks { 50000 };

namespace MQTT {
constexpr const char* server { "116.202.96.181:1883" };
constexpr int timeout { 30000 };
constexpr int qos { 1 };
constexpr int keepalive_interval { 45 };
}
namespace Log {
constexpr int interval { 1 };
}
namespace Upload {
constexpr int reminder { 5 };
constexpr std::size_t timeout { 600000UL };
constexpr const char* url { "balu.physik.uni-giessen.de:/cosmicshower" };
constexpr int port { 35221 };
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
namespace Voltage {
constexpr float bias {0.0};
constexpr float threshold[2] {0.0, 0.0};
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

#endif // MUONPI_CONFIG_H
