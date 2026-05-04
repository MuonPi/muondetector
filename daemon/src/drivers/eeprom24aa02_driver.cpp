

#include "drivers/eeprom24aa02_driver.h"

#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/registries/device_registry.h"
#include "hardware/devices.h"
#include "hardware/i2c/eeprom24aa02.h"
#include "hardware/i2c/i2cutil.h"
#include "hardware/i2cdevice_wrapper.h"
#include "sources/source.h"
#include "utility/calibration.h"

#include <chrono>
#include <cstdint>
#include <format>
#include <set>
#include <sstream>

EEPROM24AA02Driver::EEPROM24AA02Driver(ComponentId id, DeviceRegistry& registry, EventBus& bus)
    : Source(id), registry_(registry), bus_(bus) {
}

template <typename T>
std::string to_hex(T value) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << std::setw(sizeof(T) * 2) << std::setfill('0')
        << static_cast<uint64_t>(value);
    return oss.str();
}

void EEPROM24AA02Driver::update() {
    if (!std::holds_alternative<Device>(id())) {
        throw std::logic_error("DeviceComponent constructed with non-device ID");
    }
    auto* wrapper = registry_.get<I2CDeviceWrapper<EEPROM24AA02>>(std::get<Device>(id()));
    if (!wrapper) {
        return;
    }

    auto& eep24aa02 = wrapper->device();

    // Read the eeprom
    // // EEPROM 24AA02 type

    ShowerDetectorCalib calib{eep24aa02};
    if (eep24aa02.probeDevicePresence()) {
        calib.readFromEeprom();
        uint64_t id = calib.getSerialID();
        std::string hwIdStr = to_hex(id);
        // logParameter(LogParameter("uniqueId", hwIdStr, LogParameter::LOG_ONCE));
        logInfo("EEP unique ID: " + hwIdStr);
    }
    CalibStruct verStruct = calib.getCalibItem("VERSION");
    unsigned int version = 0;
    ShowerDetectorCalib::getValueFromString(verStruct.value, version);
    MuonPi::Version::hardware.major = version;
    logInfo("Found HW version " + std::to_string(MuonPi::Version::hardware.major) + " in eeprom");
    bus_.publish<ShowerDetectorCalib>(std::move(calib));
}
