#ifndef EEPROM24AA02_DRIVER_H
#define EEPROM24AA02_DRIVER_H

#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "hardware/i2c/eeprom24aa02.h"
#include "hardware/i2cdevice_wrapper.h"
#include "sources/source.h"

class EEPROM24AA02Driver : public Source {
  public:
    EEPROM24AA02Driver(ComponentId id, DeviceRegistry& registry, EventBus& bus);

    void update() override;

  private:
  private:
    DeviceRegistry& registry_;
    EventBus& bus_;

    Device deviceId_;

    std::chrono::milliseconds interval_;
    int remaining_ = 0;
};

#endif // EEPROM24AA02_DRIVER_H