#ifndef DEVICE_FACTORY_H
#define DEVICE_FACTORY_H


#include "core/registries/device_registry.h"
#include "hardware/i2cdevices.h"

#include <memory>
#include <cstdint>
#include <string>

class DeviceFactory
{
public:
    static void createADS1115(
        std::unique_ptr<DeviceRegistry>& registry,
        uint32_t id,
        const std::string& bus,
        uint8_t address);
};

#endif // DEVICE_FACTORY_H