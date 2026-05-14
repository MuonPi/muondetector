#ifndef COMPONENT_H
#define COMPONENT_H

#include "hardware/devices.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>

enum class OtherComponent : std::uint32_t {
    GPS_DRIVER_0,
    TCP_SOURCE_0,
    TCP_COMMAND_DECODER_0,
    GPIO_DRIVER_0,
};

using ComponentId = std::variant<Device, OtherComponent>;

inline const std::unordered_map<std::string, ComponentId> componentLookup{
    {"ADC_DRIVER_0", Device::ADS1115_0},
    {"DAC_DRIVER_0", Device::MCP4728_0},
    {"PCA9536_DRIVER_0", Device::PCA9536_0},
    {"EEPROM_DRIVER_0", Device::EEPROM24AA02_0},
    {"OLED_DRIVER_0", Device::ADAFRUIT_SSD1306_0},
    {"TEMP_SOURCE_0", Device::LM75_0},
    {"TEMP_SOURCE_1", Device::MIC184_0},
    {"GPS_DRIVER_0", OtherComponent::GPS_DRIVER_0},
    {"GPIO_DRIVER_0", OtherComponent::GPIO_DRIVER_0},
    {"TCP_SOURCE_0", OtherComponent::TCP_SOURCE_0},
    {"TCP_COMMAND_DECODER_0", OtherComponent::TCP_COMMAND_DECODER_0},
};

class Component {
  public:
    Component(const ComponentId id);
    virtual ~Component() = default;
    auto id() const noexcept -> ComponentId;
    auto name() const -> std::string;

    static void handleDeviceMissing(const ComponentId id);

  private:
    ComponentId id_;
};

#endif // COMPONENT_H
