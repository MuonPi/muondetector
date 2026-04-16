#ifndef MCP4728_DRIVER_H
#define MCP4728_DRIVER_H

#include "core/component.h"
#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "hardware/i2cdevice_wrapper.h"
#include "hardware/i2c/mcp4728.h"

#include "data/commands/threshold_setting_cmd.h"

#include <optional>

class MCP4728Driver : public Component
{
  public:
    MCP4728Driver(ComponentId id, DeviceRegistry &registry, EventBus &bus);

  private:
    auto dev() -> MCP4728*;
    void setThreshold(const ThresholdSettingCmd& cmd);
    DeviceRegistry &registry_;
    EventBus &bus_;
};

#endif // MCP4728_DRIVER_H