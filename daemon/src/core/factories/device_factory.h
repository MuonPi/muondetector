#ifndef DEVICE_FACTORY_H
#define DEVICE_FACTORY_H

#include "core/registries/device_registry.h"
#include "hardware/devices.h"
#include "hardware/i2cdevices.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>

using DeviceCreator = std::function<std::unique_ptr<IDevice>(const DeviceConfig &)>;
class DeviceFactory
{
  public:
    static auto create(const DeviceConfig &config) -> std::unique_ptr<IDevice>;
    static const std::unordered_map<Device, DeviceCreator> deviceCreator;
};

#endif // DEVICE_FACTORY_H