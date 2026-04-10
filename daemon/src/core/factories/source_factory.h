#ifndef SOURCE_FACTORY_H
#define SOURCE_FACTORY_H


#include "core/registries/source_manager.h"
#include "core/registries/device_registry.h"
#include "sources/i2c_sources/ads1115_source.h"
#include "sources/tcp_source.h"
#include "hardware/devices.h"


class SourceFactory
{
public:
    static auto createADS1115Source(
        Device id,
        std::unique_ptr<DeviceRegistry>& registry,
        std::unique_ptr<EventBus>& bus) -> std::shared_ptr<ADS1115Source>;

    static auto createTcpSource(
        std::unique_ptr<SourceManager>& sources,
        std::unique_ptr<EventBus>& bus) -> std::shared_ptr<TcpSource>;
};

#endif // SOURCE_FACTORY_H
