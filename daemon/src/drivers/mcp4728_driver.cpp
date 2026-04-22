#include "drivers/mcp4728_driver.h"

#include "app/system_config.h"
#include "config.h"
#include "core/logging/logger.h"
#include "data/commands/threshold_setting_cmd.h"
#include "data/events/mcp4728_event.h"
#include "data/events/threshold_setting_event.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <type_traits>
#include <variant>

using namespace MuonPi;

MCP4728Driver::MCP4728Driver(ComponentId id, SystemConfig& systemConfig, DeviceRegistry& registry,
                             EventBus& bus)
    : Component(id), registry_{registry}, bus_{bus} {
    bus_.subscribe<ThresholdSettingCmd>(
        ([this](const ThresholdSettingCmd& cmd) { setThreshold(cmd); }));

    auto* device = dev();
    if (!device || !device->probeDevicePresence()) {
        handleDeviceMissing(id);
    } else {
        MCP4728Event event;
        std::unordered_map<std::uint8_t, MCP4728::DacChannel> data;
        std::unordered_map<std::uint8_t, MCP4728::DacChannel> eepromData;
        for (std::uint8_t channel = 0; channel < 4; channel++) {
            MCP4728::DacChannel dacChannel;
            MCP4728::DacChannel eepromChannel;
            eepromChannel.eeprom = true;

            device->readChannel(channel, dacChannel);
            device->readChannel(channel, eepromChannel);
            std::stringstream sstr;
            sstr << " ch" << static_cast<unsigned>(channel) << ": " << dacChannel.value << "="
                 << MCP4728::code2voltage(dacChannel) << "V"
                 << "  (stored:" << eepromChannel.value << "="
                 << MCP4728::code2voltage(eepromChannel) << "V)";
            logInfo(sstr.str());
            event.dacValues.emplace(channel, dacChannel.value);
            event.eepromValues.emplace(channel, eepromChannel.value);
            event.voltages.emplace(channel, MCP4728::code2voltage(dacChannel));
            data.emplace(channel, std::move(dacChannel));
            eepromData.emplace(channel, std::move(eepromChannel));
        }
        for (std::uint8_t channel = 0; channel < 2; channel++) {
            if (systemConfig.thresholdVoltage[Config::Hardware::DAC::Channel::threshold[channel]] <
                0.) {
                systemConfig.thresholdVoltage[Config::Hardware::DAC::Channel::threshold[channel]] =
                    MCP4728::code2voltage(data.at(channel));
            }
        }
        const auto& eepromBias = eepromData.at(Config::Hardware::DAC::Channel::bias);
        if (eepromBias.value == 0) {
            // Set default values if nothing is set in the configuration
            device->setVoltage(Config::Hardware::DAC::Channel::bias,
                               Config::Hardware::DAC::Voltage::bias);
            device->setVoltage(Config::Hardware::DAC::Channel::threshold[0],
                               Config::Hardware::DAC::Voltage::threshold[0]);
            device->setVoltage(Config::Hardware::DAC::Channel::threshold[1],
                               Config::Hardware::DAC::Voltage::threshold[1]);
        }

        if (systemConfig.biasVoltage < 0.) {
            MCP4728::DacChannel dacChannel;
            MCP4728::DacChannel eepromChannel;
            eepromChannel.eeprom = true;
            device->readChannel(Config::Hardware::DAC::Channel::bias, dacChannel);
            device->readChannel(Config::Hardware::DAC::Channel::bias, eepromChannel);
            data.at(Config::Hardware::DAC::Channel::bias) = dacChannel;
            eepromData.at(Config::Hardware::DAC::Channel::bias) = eepromChannel;
            systemConfig.biasVoltage = MCP4728::code2voltage(dacChannel);
        }

        bus_.publish(event);
    }
}

auto MCP4728Driver::dev() -> MCP4728* {
    if (!std::holds_alternative<Device>(id()))
        throw std::logic_error("Expected Device id");
    auto* wrapper = registry_.get<I2CDeviceWrapper<MCP4728>>(std::get<Device>(id()));
    if (!wrapper) {
        return nullptr;
    }

    return &wrapper->device();
}

void MCP4728Driver::setThreshold(const ThresholdSettingCmd& cmd) {
    if (auto* dac = dev()) {
        bool success = dac->setVoltage(cmd.channel, cmd.threshold);
        bus_.publish(ThresholdSettingEvent{cmd.channel, cmd.threshold, success});
    }
}