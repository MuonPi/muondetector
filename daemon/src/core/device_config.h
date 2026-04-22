#ifndef DEVICE_CONFIG_H
#define DEVICE_CONFIG_H

#include "hardware/devices.h"

#include <optional>
#include <string>

struct DeviceConfig {
    Device id;
    DeviceType type{DeviceType::OTHER};
    std::string category;
    std::optional<std::string> device;

    // i2c
    std::optional<std::uint8_t> address{std::nullopt};

    // uart
    std::optional<std::uint32_t> baud{std::nullopt};
};

#endif // DEVICE_CONFIG_H