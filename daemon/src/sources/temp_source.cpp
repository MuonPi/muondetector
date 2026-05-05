#include "sources/temp_source.h"

#include <stdexcept>

TempSource::TempSource(ComponentId id, EventBus& bus) : Source::Source(id), bus_(bus) {
    if (!std::holds_alternative<Device>(id)) {
        throw std::logic_error("DeviceSource constructed with non-device ID");
    }
}

void TempSource::update() {
}