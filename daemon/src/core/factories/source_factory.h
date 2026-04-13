#ifndef SOURCE_FACTORY_H
#define SOURCE_FACTORY_H

#include "core/registries/device_registry.h"
#include "core/registries/source_manager.h"
#include "core/source_config.h"
#include "core/event_bus.h"
#include "hardware/devices.h"


using SourceCreator = std::function<std::shared_ptr<Source>(const SourceConfig&, DeviceRegistry &, EventBus &)>;
using DeviceSourceCreator = std::function<std::shared_ptr<Source>(const SourceConfig&, DeviceRegistry &, EventBus &)>;

class SourceFactory
{
  public:
    static auto createSource(const SourceConfig& config, DeviceRegistry &registry, EventBus &bus) -> std::shared_ptr<Source>;

  private:
    static const std::unordered_map<SourceId, SourceCreator> sourceCreator;
    static const std::unordered_map<SourceId, DeviceSourceCreator> deviceSourceCreator;
};

#endif // SOURCE_FACTORY_H
