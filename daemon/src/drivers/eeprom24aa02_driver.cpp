

#include "drivers/eeprom24aa02_driver.h"

#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/device_registry.h"
#include "data/events/calib_event.h"
#include "data/events/datastore_store_event.h"
#include "hardware/devices.h"
#include "hardware/i2c/eeprom24aa02.h"
#include "hardware/i2c/i2cutil.h"
#include "hardware/i2cdevice_wrapper.h"
#include "sources/source.h"
#include "utility/calibration.h"
#include "utility/conversion.h"

#include <chrono>
#include <cstdint>
#include <format>
#include <memory>
#include <set>
#include <sstream>

EEPROM24AA02Driver::EEPROM24AA02Driver(ComponentId id, DeviceRegistry& registry, EventBus& bus)
    : Source(id), registry_(registry), bus_(bus) {
    if (!std::holds_alternative<Device>(id)) {
        throw std::logic_error("DeviceComponent constructed with non-device ID");
    }

    auto* wrapper = registry_.get<I2CDeviceWrapper<EEPROM24AA02>>(std::get<Device>(id));
    if (!wrapper) {
        return;
    }

    auto& eep24aa02 = wrapper->device();
    if (!eep24aa02.probeDevicePresence()) {
        return;
    }

    // Read the eeprom
    // // EEPROM 24AA02 type
    calib = std::make_shared<ShowerDetectorCalib>(eep24aa02);
    calib->readFromEeprom();
    std::string hwIdStr = to_hex(calib->getSerialID());
    logInfo("EEP unique ID: " + hwIdStr);
    CalibStruct verStruct = calib->getCalibItem("VERSION");
    unsigned int version = 0;
    ShowerDetectorCalib::getValueFromString(verStruct.value, version);
    MuonPi::Version::hardware.major = version;
    logInfo("Found HW version " + std::to_string(MuonPi::Version::hardware.major) + " in eeprom");
    bus_.publish(DatastoreStoreEvent{std::weak_ptr<ShowerDetectorCalib>(calib)});
}

void EEPROM24AA02Driver::update() {
    if (calib != nullptr) {
        logInfo("calib pointer is valid");
        bus_.publish(DatastoreStoreEvent{CalibEvent{.valid = calib->isValid(),
                                                    .eepromValid = calib->isEepromValid(),
                                                    .id = calib->getSerialID(),
                                                    .calibList = calib->getCalibList()}});
    } else {
        logInfo("calib pointer is NOT valid");
    }
}
