#ifndef DEVICES_H
#define DEVICES_H

#include "hardware/device_types.h"
#include <cstdint>
#include <map>


enum class Device : std::uint32_t
{
    ADS1115_0,
    MCP4728_0
};

inline const std::map<std::string, Device> deviceLookup = {{"ADS1115_0", Device::ADS1115_0}, {"MCP4728_0", Device::MCP4728_0}};

#endif // DEVICES_H