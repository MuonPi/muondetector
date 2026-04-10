#include "device_factory.h"
#include "hardware/devices.h"
#include "hardware/i2cdevice_wrapper.h"
#include "hardware/i2c/ads1115.h"

#include <memory>
#include <cstdint>
#include <string>

auto DeviceFactory::createADS1115(
    const std::string& bus,
    std::uint8_t address) -> std::unique_ptr<IDevice>
{
    return std::make_unique<I2CDeviceWrapper<ADS1115>>(
        std::make_unique<ADS1115>(bus.c_str(), address)
    );
}