#include "device_factory.h"
#include "core/logging/logger.h"
#include "hardware/devices.h"
#include "hardware/i2c/ads1115.h"
#include "hardware/i2cdevice_wrapper.h"

#include <cstdint>
#include <memory>
#include <string>

auto DeviceFactory::create(const DeviceConfig &config) -> std::unique_ptr<IDevice>
{
    auto it = DeviceFactory::deviceCreator.find(config.id);
    if (it == DeviceFactory::deviceCreator.end()) {
        throw std::runtime_error("No factory method found for creation of device " + std::to_string(static_cast<unsigned>(config.id)));
    }
    return it->second(config);
}

const std::unordered_map<Device, DeviceCreator>
DeviceFactory::deviceCreator =
{
    {
        Device::ADS1115_0,
        [](const DeviceConfig& cfg)
        {
            return std::make_unique<I2CDeviceWrapper<ADS1115>>(
                std::make_unique<ADS1115>(cfg.device.value().c_str(), cfg.address.value())
            );
        }
    },
    {
        Device::MCP4728_0,
        [](const DeviceConfig& cfg)
        {
            return std::make_unique<I2CDeviceWrapper<MCP4728>>(
                std::make_unique<MCP4728>(cfg.device.value().c_str(), cfg.address.value())
            );
        }
    }
};