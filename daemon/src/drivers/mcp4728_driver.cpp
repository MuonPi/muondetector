#include "drivers/mcp4728_driver.h"

#include "app/system_config.h"
#include "config.h"
#include "core/logging/logger.h"
// #include "data/commands/threshold_setting_cmd.h"
#include "data/commands/bias_dac_setting_cmd.h"
#include "data/commands/dac_cmd.h"
#include "data/events/bias_voltage_event.h"
#include "data/events/mcp4728_event.h"
#include "data/events/threshold_setting_event.h"
#include "hardware/i2c/mcp4728.h"
#include "hardware/i2cdevice_wrapper.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <type_traits>
#include <variant>

using namespace MuonPi;

MCP4728Driver::MCP4728Driver(ComponentId id, SystemConfig& systemConfig, DeviceRegistry& registry,
                             EventBus& bus)
    : Component(id), registry_{registry}, bus_{bus} {
    if (!std::holds_alternative<Device>(id)) {
        throw std::logic_error("DeviceComponent constructed with non-device ID");
    }

    auto* device = dev();
    if (device == nullptr) {
        return;
    }

    // This one is not used since all dac settings are requested at once
    // bus_.subscribe<ThresholdSettingRequestCmd>(
    //     [this](const ThresholdSettingRequestCmd& cmd) { });

    bus_.subscribe<DacRequestCmd>([this]([[maybe_unused]] const DacRequestCmd&) {
        auto* device = dev();
        if (device == nullptr) {
            return;
        }
        bus_.publish(readDac(device));
    });
    bus_.subscribe<ThresholdSettingCmd>(
        [this](const ThresholdSettingCmd& cmd) { setThreshold(cmd); });
    bus_.subscribe<BiasVoltageCmd>([this](const BiasVoltageCmd& cmd) { setBiasVoltage(cmd); });

    auto event = readAll(device);

    for (std::uint8_t channel = 0; channel < 2; channel++) {
        if (systemConfig.thresholdVoltage[channel] < 0.) {
            systemConfig.thresholdVoltage[channel] =
                event.voltages.at(Config::Hardware::DAC::Channel::threshold[channel]);
        }
    }

    // Check if there is a value for bias in eeprom
    const auto& eepromBias = event.eepromValues.value().at(Config::Hardware::DAC::Channel::bias);
    if (eepromBias == 0) {
        // Set default values if nothing is set in the eeprom configuration
        device->setVoltage(Config::Hardware::DAC::Channel::bias,
                           Config::Hardware::DAC::Voltage::bias);
        device->setVoltage(Config::Hardware::DAC::Channel::threshold[0],
                           Config::Hardware::DAC::Voltage::threshold[0]);
        device->setVoltage(Config::Hardware::DAC::Channel::threshold[1],
                           Config::Hardware::DAC::Voltage::threshold[1]);
        // Re-read
        event = readAll(device);
    }

    // If bias voltage not set in systemConfig, set it
    if (systemConfig.biasVoltage < 0.) {
        systemConfig.biasVoltage = event.voltages.at(Config::Hardware::DAC::Channel::bias);
    }

    // I don't know why thresholds are set here again
    if (systemConfig.thresholdVoltage[0] > 0.) {
        device->setVoltage(Config::Hardware::DAC::Channel::threshold[0],
                           systemConfig.thresholdVoltage[0]);
    }
    if (systemConfig.thresholdVoltage[1] > 0.) {
        device->setVoltage(Config::Hardware::DAC::Channel::threshold[1],
                           systemConfig.thresholdVoltage[1]);
    }
    if (systemConfig.biasVoltage > 0.) {
        device->setVoltage(Config::Hardware::DAC::Channel::bias, systemConfig.biasVoltage);
    }

    bus_.publish(event);
}

auto MCP4728Driver::dev() -> MCP4728* {
    auto* wrapper = registry_.get<I2CDeviceWrapper<MCP4728>>(std::get<Device>(id()));
    if (!wrapper) {
        logWarn("MCP4728 Device not found");
        return nullptr;
    }

    return &wrapper->device();
}

auto MCP4728Driver::readEeprom(MCP4728* dev)
    -> std::unordered_map<std::uint8_t, MCP4728::DacChannel> {
    std::unordered_map<std::uint8_t, MCP4728::DacChannel> eepromDataForEvent;
    for (std::uint8_t channel = 0; channel < 4; channel++) {
        MCP4728::DacChannel eepromChannel;
        eepromChannel.eeprom = true;
        dev->readChannel(channel, eepromChannel);
        eepromDataForEvent.emplace(channel, eepromChannel);
    }
    return eepromDataForEvent;
}

auto MCP4728Driver::readDac(MCP4728* dev) -> MCP4728Event {
    MCP4728Event event;
    for (std::uint8_t channel = 0; channel < 4; channel++) {
        MCP4728::DacChannel dacChannel;

        dev->readChannel(channel, dacChannel);

        event.dacValues.emplace(channel, dacChannel.value);
        event.voltages.emplace(channel, MCP4728::code2voltage(dacChannel));
    }
    return event;
}

auto MCP4728Driver::readAll(MCP4728* dev) -> MCP4728Event {
    auto event = readDac(dev);
    auto eepromData = readEeprom(dev);
    for (std::uint8_t channel = 0; channel < 4; channel++) {
        std::stringstream sstr;
        sstr << " ch" << static_cast<unsigned>(channel) << ": " << event.dacValues.at(channel)
             << "=" << event.voltages.at(channel) << " V"
             << "  (stored:" << event.eepromValues.value().at(channel) << "="
             << MCP4728::code2voltage(eepromData.at(channel)) << "V)";
        logInfo(sstr.str());
    }
    std::unordered_map<std::uint8_t, std::uint16_t> converted;
    for (auto& [key, dacChannel] : eepromData) {
        converted.emplace(key, dacChannel.value);
    }
    event.eepromValues.emplace(std::move(converted));
    return event;
}

void MCP4728Driver::setThreshold(const ThresholdSettingCmd& cmd) {
    if (cmd.channel > 1) {
        logWarn("Tried to set threshold on not existing channel " + std::to_string(cmd.channel));
        return;
    }
    if (auto* dac = dev()) {
        bool success =
            dac->setVoltage(Config::Hardware::DAC::Channel::threshold[cmd.channel], cmd.threshold);
        bus_.publish(ThresholdSettingEvent{cmd.channel, cmd.threshold, success});
    }
}

void MCP4728Driver::setBiasVoltage(const BiasVoltageCmd& cmd) {
    if (auto* dac = dev()) {
        bool success = dac->setVoltage(Config::Hardware::DAC::Channel::bias, cmd.voltage);
        if (success == false) {
            logWarn("Failed to set bias voltage");
        }
        bus_.publish(BiasVoltageEvent{cmd.voltage});
    }
}
