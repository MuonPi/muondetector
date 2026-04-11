#ifndef SOURCE_FACTORY_H
#define SOURCE_FACTORY_H

#include "core/registries/device_registry.h"
#include "core/registries/source_manager.h"
#include "hardware/devices.h"
#include "sources/i2c_sources/ads1115_source.h"
#include "sources/tcp_source.h"

using DeviceSourceCreator = std::function<std::shared_ptr<Source>(Device, DeviceRegistry &, EventBus &)>;

class SourceFactory
{
  public:
    static auto createDeviceSource(Device id, DeviceRegistry &registry, EventBus &bus) -> std::shared_ptr<Source>;

    static auto createTcpSource(EventBus &bus) -> std::shared_ptr<TcpSource>;

  private:
    static const std::unordered_map<Device, DeviceSourceCreator> deviceSourceCreator;
};

#endif // SOURCE_FACTORY_H
