#ifndef DEVICES_H
#define DEVICES_H

#include "hardware/device_types.h"
#include <cstdint>
#include <map>


enum class Device : std::uint32_t
{
    ADS1115_0,
    GPS_UART_0
};

inline const std::map<std::string, Device> deviceLookup = {{"ADS1115_0", Device::ADS1115_0}, {"GPS_UART_0", Device::GPS_UART_0}};

inline const std::unordered_map<Device, std::uint32_t> deviceAddressMap = {{Device::ADS1115_0, 0x48u}};

#endif // DEVICES_H