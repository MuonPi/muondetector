#include "source_factory.h"
#include "hardware/devices.h"
#include <memory>

auto SourceFactory::createDeviceSource(Device id, DeviceRegistry &registry, EventBus &bus) -> std::shared_ptr<Source>
{
    auto it = deviceSourceCreator.find(id);

    if (it == deviceSourceCreator.end())
    {
        throw std::runtime_error("No factory method for source " + std::to_string(static_cast<unsigned>(id)));
    }

    return it->second(id, registry, bus);
}

auto SourceFactory::createTcpSource(EventBus &bus) -> std::shared_ptr<TcpSource>
{
    return std::make_shared<TcpSource>(bus);
}

const std::unordered_map<Device, DeviceSourceCreator> SourceFactory::deviceSourceCreator = {
    {Device::ADS1115_0, [](Device id, DeviceRegistry &registry, EventBus &bus) {
         return std::make_shared<ADS1115Source>(id, registry, bus);
     }}};