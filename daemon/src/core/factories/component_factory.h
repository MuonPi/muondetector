#ifndef SOURCE_FACTORY_H
#define SOURCE_FACTORY_H

#include "core/context.h"
#include "core/component_config.h"
// #include "core/registries/device_registry.h"
// #include "core/registries/source_manager.h"
// #include "core/event_bus.h"
// #include "hardware/devices.h"


using ComponentCreator = std::function<std::shared_ptr<Component>(Context& ctx)>;
using DeviceComponentCreator = std::function<std::shared_ptr<Component>(Context& ctx)>;

class ComponentFactory
{
  public:
    static auto createComponent(const ComponentConfig& config, Context& ctx) -> std::shared_ptr<Component>;

  private:
    static const std::unordered_map<ComponentId, ComponentCreator> componentCreator;
    static const std::unordered_map<ComponentId, DeviceComponentCreator> deviceComponentCreator;
};

#endif // SOURCE_FACTORY_H
