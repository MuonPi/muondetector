#include "sources/temp_source.h"

#include "core/logging/logger.h"
#include "core/registries/device_registry.h"
#include "data/events/datastore_store_event.h"
#include "data/events/lm75_event.h"
#include "hardware/i2c/lm75.h"
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
    bus_.publish(DatastoreStoreEvent{LM75Event{device->getTemperature()}});
}

auto TempSource::dev() -> LM75* {
    auto* wrapper = registry_.get<I2CDeviceWrapper<LM75>>(std::get<Device>(id()));
    if (!wrapper) {
        logWarn("LM75 Device not found");
        return nullptr;
    }

    return &wrapper->device();
}