#ifndef TEMP_SOURCE_H
#define TEMP_SOURCE_H

#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "sources/source.h"

#include <memory>

class LM75;
class TempSource : public Source {
  public:
    explicit TempSource(ComponentId id, DeviceRegistry& registry, EventBus& bus);

    void update() override;

  private:
    auto dev() -> LM75*;
    DeviceRegistry& registry_;
    EventBus& bus_;
};

#endif // TEMP_SOURCE_H