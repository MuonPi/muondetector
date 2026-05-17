#include "drivers/pca9536_driver.h"

#include "app/system_config.h"
#include "core/component.h"
#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/device_registry.h"
#include "data/commands/pca_switch_cmd.h"
#include "data/events/datastore_store_event.h"
#include "data/events/pca_switch_event.h"
#include "hardware/i2c/pca9536.h"
#include "hardware/i2cdevice_wrapper.h"
#include "utility/conversion.h"
#include "utility/logparameter.h"

PCA9536Driver::PCA9536Driver(ComponentId id, SystemConfig& systemConfig, DeviceRegistry& registry,
                             EventBus& bus)
    : Component(id), registry_(registry), bus_(bus) {
    if (!std::holds_alternative<Device>(id)) {
        throw std::logic_error("DeviceComponent constructed with non-device ID");
    }
    auto* device = dev();
    if (device == nullptr) {
        return;
    }

    device->setOutputPorts(0b00000111);

    auto channel = systemConfig.pcaPortMask;
    if (channel > ((MuonPi::Version::hardware.major == 1) ? 3 : 7)) {
        logWarn("invalid PCA channel selection: ch" + std::to_string(channel) + "...ignoring");
        return;
    }
    logDebug("changed pcaPortMask to " + std::to_string(channel));
    // systemConfig.pcaPortMask = channel;
    setOutputState(channel);
    bus_.subscribe<PcaSwitchCmd>(
        [this](const PcaSwitchCmd& cmd) { setOutputState(cmd.pcaPortMask); });
}

auto PCA9536Driver::dev() -> PCA9536* {
    auto* wrapper = registry_.get<I2CDeviceWrapper<PCA9536>>(std::get<Device>(id()));
    if (!wrapper) {
        return nullptr;
    }

    return &wrapper->device();
}

void PCA9536Driver::setOutputState(std::uint8_t pcaPortMask) {
    auto* device = dev();
    if (device == nullptr) {
        return;
    }
    device->setOutputState(pcaPortMask);
    bus_.publish(DatastoreStoreEvent{PcaSwitchEvent{.pcaPortMask = pcaPortMask}});
    bus_.publish<LogParameter>(
        LogParameter("ubxInputSwitch", "0x" + to_hex(pcaPortMask), LogParameter::LOG_EVERY));
}