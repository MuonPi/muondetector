#ifndef COMPONENT_H
#define COMPONENT_H

#include <cstdint>
#include <variant>
#include <string>
#include <unordered_map>

#include "hardware/devices.h"

enum class OtherComponent : std::uint32_t {
    GPS_DRIVER_0,
    TCP_SOURCE_0,
};

using ComponentId = std::variant<Device, OtherComponent>;


inline const std::unordered_map<std::string, ComponentId> componentLookup {
    {"ADC_DRIVER_0", Device::ADS1115_0},
    {"DAC_DRIVER_0", Device::MCP4728_0},
    {"GPS_DRIVER_0", OtherComponent::GPS_DRIVER_0},
    {"TCP_SOURCE_0", OtherComponent::TCP_SOURCE_0},
};

class Component {
public:
    Component(const ComponentId id);
    virtual ~Component() = default;
    auto id() const noexcept -> ComponentId;
    auto name() const noexcept -> std::optional<std::string>;


    static void handleDeviceMissing(const ComponentId id);

private:
    ComponentId id_;
};

#endif // COMPONENT_H