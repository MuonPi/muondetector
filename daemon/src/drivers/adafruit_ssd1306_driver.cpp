#include "drivers/adafruit_ssd1306_driver.h"

#include "app/system_config.h"
#include "core/component.h"
#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "data/events/oled_event.h"
#include "hardware/i2c/adafruit_ssd1306.h"
#include "hardware/i2cdevice_wrapper.h"

#define DEGREE_CHARCODE 248

Adafruit_SSD1306Driver::Adafruit_SSD1306Driver(ComponentId id, SystemConfig& systemConfig,
                                               DeviceRegistry& registry, EventBus& bus)
    : Component(id), registry_(registry), bus_(bus) {
    if (!std::holds_alternative<Device>(id)) {
        throw std::logic_error("DeviceComponent constructed with non-device ID");
    }

    auto device = dev();
    if (device == nullptr) {
        return;
    }

    device->begin();
    device->clearDisplay();

    // text display tests
    device->setTextSize(1);
    device->setTextColor(Adafruit_SSD1306::WHITE);
    device->setCursor(0, 2);
    device->print("*Cosmic Shower Det.*\n");
    device->print("V");
    device->print(MuonPi::Version::software.string().c_str());
    device->print("\n");
    device->display();
}

void Adafruit_SSD1306Driver::update(const OledEvent& event) {
    auto device = dev();
    if (!device || !device->devicePresent())
        return;
    device->clearDisplay();
    device->setCursor(0, 2);
    device->print("*Cosmic Shower Det.*\n");
    device->printf("Rates %4.1f %4.1f /s\n", event.andRate, event.xorRate);
    device->printf("temp %4.2f %cC\n", event.temp, DEGREE_CHARCODE);
    device->printf("%d(%d) Sats ", event.nSatsVisible, event.nSats, DEGREE_CHARCODE);
    device->printf("%s\n", Gnss::FixType::name[event.gpsFix.value]);
    device->display();
}

auto Adafruit_SSD1306Driver::dev() -> Adafruit_SSD1306* {
    auto* wrapper = registry_.get<I2CDeviceWrapper<Adafruit_SSD1306>>(std::get<Device>(id()));
    if (!wrapper) {
        return nullptr;
    }

    return &wrapper->device();
}