#ifndef DEVICE_FACTORY_H
#define DEVICE_FACTORY_H

#include "core/registries/device_registry.h"
#include "hardware/devices.h"
#include "hardware/i2cdevices.h"

#include <cstdint>
#include <memory>
#include <string>

class DeviceFactory
{
  public:
    static auto createADS1115(const std::string &bus, std::uint8_t address) -> std::unique_ptr<IDevice>;
};

#endif // DEVICE_FACTORY_H