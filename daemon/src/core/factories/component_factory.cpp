#include "component_factory.h"
#include "core/context.h"
// #include "hardware/devices.h"
#include "core/component_config.h"
// #include "core/logging/logger.h"
#include "drivers/ads1115_driver.h"
#include "sources/tcp_source.h"

#include <type_traits>
#include <variant>
#include <memory>


auto ComponentFactory::createComponent(const ComponentConfig& config, Context& ctx) -> std::shared_ptr<Component>
{
    return std::visit([&](auto&& arg){
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Device>) {
            auto it = deviceComponentCreator.find(arg);
            if (it == deviceComponentCreator.end())
            {
                throw std::runtime_error("No factory method found for component " + std::to_string(static_cast<unsigned>(arg)));
            }
            return it->second(ctx);
        }
        else if constexpr (std::is_same_v<T, NonDeviceComponent>) {
            auto it = componentCreator.find(arg);
            if (it == componentCreator.end())
            {
                throw std::runtime_error("No factory method found for component " + std::to_string(static_cast<unsigned>(arg)));
            }
            return it->second(ctx);
        }
        else {
            static_assert(false, "non-exhaustive visitor!");
        }
    }, config.id);
}

const std::unordered_map<ComponentId, DeviceComponentCreator> ComponentFactory::deviceComponentCreator = {
    {Device::ADS1115_0, [](Context& ctx) {
         return std::make_shared<ADS1115Driver>(Device::ADS1115_0, *ctx.registry, *ctx.bus, *ctx.io);
     }}};


const std::unordered_map<ComponentId, ComponentCreator> ComponentFactory::componentCreator = {
    {NonDeviceComponent::TCP_SOURCE_0, [](Context& ctx) {
         return std::make_shared<TcpSource>(NonDeviceComponent::TCP_SOURCE_0, *ctx.bus);
     }}};