#ifndef COMPONENT_H
#define COMPONENT_H

#include <cstdint>
#include <variant>
#include <string>
#include <unordered_map>

#include "hardware/devices.h"

enum class NonDeviceComponent : std::uint32_t {
    TCP_SOURCE_0
};

using ComponentId = std::variant<Device, NonDeviceComponent>;


inline const std::unordered_map<std::string, ComponentId> componentLookup {
    {"ADC_SOURCE_0", Device::ADS1115_0},
    {"GPS_SOURCE_0", Device::GPS_UART_0},
    {"TCP_SOURCE_0", NonDeviceComponent::TCP_SOURCE_0}
};

class Component {
public:
    Component(ComponentId id);
    virtual ~Component() = default;
    auto id() -> ComponentId;

protected:
    ComponentId m_id;
};

#endif // COMPONENT_H