#ifndef COMPONENT_CONFIG_H
#define COMPONENT_CONFIG_H

#include "sources/source.h"
#include "hardware/devices.h"

#include <string>
#include <optional>


struct ComponentConfig
{
    ComponentId id;
    std::optional<Device> deviceId{std::nullopt};
    std::optional<std::chrono::milliseconds> interval{std::nullopt};
};

#endif // COMPONENT_CONFIG_H