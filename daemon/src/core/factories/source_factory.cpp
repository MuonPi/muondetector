#include "source_factory.h"
#include "hardware/devices.h"
#include "core/source_config.h"
#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "sources/i2c_sources/ads1115_source.h"
#include "sources/tcp_source.h"

#include <type_traits>
#include <variant>
#include <memory>


auto SourceFactory::createSource(const SourceConfig& config, DeviceRegistry &registry, EventBus &bus) -> std::shared_ptr<Source>
{
    return std::visit([&](auto&& arg){
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, Device>) {
            auto it = deviceSourceCreator.find(arg);
            if (it == deviceSourceCreator.end())
            {
                throw std::runtime_error("No factory method found for source " + std::to_string(static_cast<unsigned>(arg)));
            }
            return it->second(config, registry, bus);
        }
        else if constexpr (std::is_same_v<T, NonDeviceSource>) {
            auto it = sourceCreator.find(arg);
            if (it == sourceCreator.end())
            {
                throw std::runtime_error("No factory method found for source " + std::to_string(static_cast<unsigned>(arg)));
            }
            return it->second(config, registry, bus);
        }
        else {
            static_assert(false, "non-exhaustive visitor!");
        }
    }, config.id);
}

const std::unordered_map<SourceId, SourceCreator> SourceFactory::deviceSourceCreator = {
    {Device::ADS1115_0, []([[maybe_unused]] const SourceConfig& config, DeviceRegistry &registry, EventBus &bus) {
         return std::make_shared<ADS1115Source>(Device::ADS1115_0, registry, bus);
     }}};


const std::unordered_map<SourceId, SourceCreator> SourceFactory::sourceCreator = {
    {NonDeviceSource::TCP_SOURCE_0, []([[maybe_unused]] const SourceConfig&, [[maybe_unused]] DeviceRegistry &, EventBus &bus) {
         return std::make_shared<TcpSource>(NonDeviceSource::TCP_SOURCE_0, bus);
     }}};