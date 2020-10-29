#ifndef MUONPI_CONFIG_H
#define MUONPI_CONFIG_H

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
    software { 1, 2, 0 };
}

namespace MuonPi::Config {
constexpr const char* file { "/etc/muondetector/muondetector.conf" };

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
