#ifndef SOURCE_H
#define SOURCE_H

#include <cstdint>
#include <variant>
#include "hardware/devices.h"

enum class NonDeviceSource : std::uint32_t {
    TCP_SOURCE_0
};

using SourceId = std::variant<Device, NonDeviceSource>;


inline const std::unordered_map<std::string, SourceId> sourceLookup {
    {"ADC_SOURCE_0", Device::ADS1115_0},
    {"GPS_SOURCE_0", Device::GPS_UART_0},
    {"TCP_SOURCE_0", NonDeviceSource::TCP_SOURCE_0}
};
class Source
{
public:
    Source(SourceId id);
    virtual ~Source() = default;
    virtual void update() = 0;
    auto id() -> SourceId;
protected:
    SourceId m_id;
};

#endif // SOURCE_H