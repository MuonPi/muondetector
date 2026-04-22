#ifndef SYSTEM_BUILDER_H
#define SYSTEM_BUILDER_H

#include "core/context.h"

struct DeviceConfig;
struct ComponentConfig;
class SystemBuilder {
  public:
    static auto
    parseHardwareConfig(const libconfig::Config& hardwareConfig) -> std::vector<DeviceConfig>;
    static auto
    parseComponentConfig(const libconfig::Config& componentConfig) -> std::vector<ComponentConfig>;
    static Context build(ThreadPool& pool, const SystemConfig& config);
};

#endif // SYSTEM_BUILDER_H