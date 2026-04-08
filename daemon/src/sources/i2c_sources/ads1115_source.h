
#include "sources/source.h"
#include "core/registries/device_registry.h"
#include "core/event_bus.h"
#include "data/ad1115_event.h"
#include "hardware/devices.h"

// your real device
#include "hardware/i2c/ads1115.h"
#include <cstdint>

class ADS1115Source : public Source
{
public:
    ADS1115Source(SourceId id,
                  DeviceRegistry& registry,
                  EventBus& bus);

    void update() override;
private:
    DeviceRegistry& m_registry;
    EventBus& m_bus;
};
