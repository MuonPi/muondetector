#include "device_factory.h"
#include "hardware/i2c/ads1115.h"

#include <memory>
#include <cstdint>
#include <string>

void DeviceFactory::createADS1115(
    std::unique_ptr<DeviceRegistry>& registry,
    std::uint32_t id,
    const std::string& bus,
    std::uint8_t address)
{
    registry->emplace<ADS1115>(id, bus.c_str(), address);
}