#include "source_factory.h"
#include "hardware/devices.h"
#include <memory>

auto SourceFactory::createADS1115Source(
        Device id,
        std::unique_ptr<DeviceRegistry>& registry,
        std::unique_ptr<EventBus>& bus) -> std::shared_ptr<ADS1115Source>
{
    return std::make_shared<ADS1115Source>(id, *registry, *bus);
}

auto SourceFactory::createTcpSource(
        std::unique_ptr<SourceManager>& sources,
        std::unique_ptr<EventBus>& bus) -> std::shared_ptr<TcpSource>
{
    return std::make_shared<TcpSource>(*bus);
}
