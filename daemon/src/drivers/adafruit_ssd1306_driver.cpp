#include "drivers/adafruit_ssd1306_driver.h"

#include "app/system_config.h"
#include "core/component.h"
#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "hardware/i2c/adafruit_ssd1306.h"
#include "hardware/i2cdevice_wrapper.h"

Adafruit_SSD1306Driver::Adafruit_SSD1306Driver(ComponentId id, SystemConfig& systemConfig,
                                               DeviceRegistry& registry, EventBus& bus)
    : Component(id), registry_(registry), bus_(bus) {
    if (!std::holds_alternative<Device>(id)) {
        throw std::logic_error("DeviceComponent constructed with non-device ID");
    }
}

auto Adafruit_SSD1306Driver::dev() -> Adafruit_SSD1306* {
    auto* wrapper = registry_.get<I2CDeviceWrapper<Adafruit_SSD1306>>(std::get<Device>(id()));
    if (!wrapper) {
        return nullptr;
    }

    return &wrapper->device();
}