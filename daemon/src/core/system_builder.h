#ifndef SYSTEM_BUILDER_H
#define SYSTEM_BUILDER_H

#include "core/context.h"

struct DeviceConfig;
struct ComponentConfig;
class SystemBuilder {
  public:
    static Context build(ThreadPool& pool, const SystemConfig& config);

  private:
    static auto
    parseHardwareConfig(const libconfig::Config& hardwareConfig) -> std::vector<DeviceConfig>;
    static auto
    loadHardwareConfig(const std::string& hardwareConfigPath) -> std::vector<DeviceConfig>;
    static auto
    parseComponentConfig(const libconfig::Config& componentConfig) -> std::vector<ComponentConfig>;
    static auto
    loadComponentConfig(const std::string& componentConfigPath) -> std::vector<ComponentConfig>;
};

#endif // SYSTEM_BUILDER_H