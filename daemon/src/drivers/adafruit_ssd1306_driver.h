#ifndef ADAFRUIT_SSD1306_DRIVER_H
#define ADAFRUIT_SSD1306_DRIVER_H

#include "app/system_config.h"
#include "core/component.h"
#include "core/event_bus.h"
#include "core/registries/device_registry.h"

class Adafruit_SSD1306;

class Adafruit_SSD1306Driver : public Component {
  public:
    Adafruit_SSD1306Driver(ComponentId id, SystemConfig& systemConfig, DeviceRegistry& registry,
                           EventBus& bus);

  private:
    auto dev() -> Adafruit_SSD1306*;
    DeviceRegistry& registry_;
    EventBus& bus_;
};

#endif // ADAFRUIT_SSD1306_DRIVER_H