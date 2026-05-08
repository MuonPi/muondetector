#include "drivers/pca9536_driver.h"

#include "app/system_config.h"
#include "core/component.h"
#include "core/event_bus.h"
#include "core/registries/device_registry.h"
#include "hardware/i2c/pca9536.h"
#include "hardware/i2cdevice_wrapper.h"

PCA9536Driver::PCA9536Driver(ComponentId id, SystemConfig& systemConfig, DeviceRegistry& registry,
                             EventBus& bus)
    : Component(id), registry_(registry), bus_(bus) {
    if (!std::holds_alternative<Device>(id)) {
        throw std::logic_error("DeviceComponent constructed with non-device ID");
    }
}

auto PCA9536Driver::dev() -> PCA9536* {
    auto* wrapper = registry_.get<I2CDeviceWrapper<PCA9536>>(std::get<Device>(id()));
    if (!wrapper) {
        return nullptr;
    }

    return &wrapper->device();
}
