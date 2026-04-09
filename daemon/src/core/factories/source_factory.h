#ifndef SOURCE_FACTORY_H
#define SOURCE_FACTORY_H


#include "core/registries/source_manager.h"
#include "core/registries/device_registry.h"
#include "sources/i2c_sources/ads1115_source.h"


class SourceFactory
{
public:
    static void createADS1115Source(
        std::unique_ptr<SourceManager>& sources,
        uint32_t id,
        std::unique_ptr<DeviceRegistry>& registry,
        std::unique_ptr<EventBus>& bus);
};

#endif // SOURCE_FACTORY_H