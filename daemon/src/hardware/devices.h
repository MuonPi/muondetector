#ifndef DEVICES_H
#define DEVICES_H

#include <cstdint>
#include <unordered_map>

enum class Device : std::uint32_t
{
    AD1115
};


inline const std::unordered_map<Device, std::uint32_t> deviceAddressMap = {
    {Device::AD1115, 0x48u}
};


#endif // DEVICES_H