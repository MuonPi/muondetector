#include "source_factory.h"
#include <memory>

void SourceFactory::createADS1115Source(
        std::unique_ptr<SourceManager>& sources,
        uint32_t id,
        std::unique_ptr<DeviceRegistry>& registry,
        std::unique_ptr<EventBus>& bus)
{
    sources->add(std::make_unique<ADS1115Source>(id, *registry, *bus));
}
