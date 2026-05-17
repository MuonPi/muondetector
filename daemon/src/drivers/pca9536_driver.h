#ifndef PCA9536_DRIVER_H
#define PCA9536_DRIVER_H

#include "app/system_config.h"
#include "core/component.h"
#include "core/event_bus.h"
#include "core/registries/device_registry.h"

class PCA9536;

class PCA9536Driver : public Component {

  public:
    PCA9536Driver(ComponentId id, SystemConfig& systemConfig, DeviceRegistry& registry,
                  EventBus& bus);

  private:
    auto dev() -> PCA9536*;
    void setOutputState(std::uint8_t pcaPortMask);
    DeviceRegistry& registry_;
    EventBus& bus_;
};
#endif // PCA9536_DRIVER_H