#include "drivers/mcp4728_driver.h"
#include "data/commands/threshold_setting_cmd.h"
#include "data/events/threshold_setting_event.h"
#include "core/logging/logger.h"

#include <optional>
#include <algorithm>
#include <type_traits>
#include <variant>


MCP4728Driver::MCP4728Driver(ComponentId id, DeviceRegistry &registry, EventBus &bus)
    : Component(id), registry_{registry}, bus_{bus}
{
    bus_.subscribe<ThresholdSettingCmd>(([this](const ThresholdSettingCmd &cmd) { setThreshold(cmd); }));

    if (auto* device = dev(); !device || !device->probeDevicePresence())
    {
        handleDeviceMissing(id);
    }
}

auto MCP4728Driver::dev() -> MCP4728*
{
    if (!std::holds_alternative<Device>(id()))
        throw std::logic_error("Expected Device id");
    auto* wrapper = registry_.get<I2CDeviceWrapper<MCP4728>>(std::get<Device>(id()));
    if (!wrapper) {
        return nullptr;
    }

    return &wrapper->device();
}

void MCP4728Driver::setThreshold(const ThresholdSettingCmd& cmd)
{
    if (auto* dac = dev())
    {
        bool success = dac->setVoltage(cmd.channel, cmd.threshold);
        bus_.publish(ThresholdSettingEvent{
            cmd.channel,
            cmd.threshold,
            success
        });
    }
}