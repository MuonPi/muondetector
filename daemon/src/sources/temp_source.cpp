#include "sources/temp_source.h"

#include "core/logging/logger.h"
#include "core/registries/device_registry.h"
#include "data/events/datastore_store_event.h"
#include "data/events/temperature_event.h"
#include "hardware/i2c/lm75.h"
#include "hardware/i2c/mic184.h"
#include "hardware/i2cdevice_wrapper.h"

#include <stdexcept>

TempSource::TempSource(ComponentId id, DeviceRegistry& registry, EventBus& bus)
    : Source::Source(id), registry_{registry}, bus_(bus) {
    if (!std::holds_alternative<Device>(id)) {
        throw std::logic_error("DeviceSource constructed with non-device ID");
    }
}

void TempSource::update() {
    auto device = dev();
    if (device == nullptr) {
        return;
    }
    bus_.publish(DatastoreStoreEvent{TemperatureEvent{name(), device->getTemperature()}});
}

auto TempSource::dev() -> DeviceFunction<DeviceType::TEMP>* {
    const auto deviceId = std::get<Device>(id());
    if (auto* wrapper = registry_.get<I2CDeviceWrapper<LM75>>(deviceId)) {
        return &wrapper->device();
    }

    if (auto* wrapper = registry_.get<I2CDeviceWrapper<MIC184>>(deviceId)) {
        return &wrapper->device();
    }

    logError("Temperature device not found");
    return nullptr;
}
