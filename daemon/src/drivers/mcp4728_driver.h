#ifndef MCP4728_DRIVER_H
#define MCP4728_DRIVER_H

#include "app/system_config.h"
#include "core/component.h"
#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "data/commands/bias_dac_setting_cmd.h"
#include "data/commands/threshold_setting_cmd.h"
#include "data/events/mcp4728_event.h"

// Not optimum here
#include "hardware/i2c/mcp4728.h"

#include <optional>

// class MCP4728;

class MCP4728Driver : public Component {
  public:
    MCP4728Driver(ComponentId id, SystemConfig& systemConfig, DeviceRegistry& registry,
                  EventBus& bus);

  private:
    auto dev() -> MCP4728*;
    static auto readEeprom(MCP4728* dev) -> std::unordered_map<std::uint8_t, MCP4728::DacChannel>;
    static auto readDac(MCP4728* dev) -> MCP4728Event;
    static auto readAll(MCP4728* dev) -> MCP4728Event;
    void setThreshold(const ThresholdSettingCmd& cmd);
    void setBiasVoltage(const BiasVoltageCmd& cmd);
    DeviceRegistry& registry_;
    EventBus& bus_;
};

#endif // MCP4728_DRIVER_H