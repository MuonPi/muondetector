#include "i2c/sen0321.h"

SEN0321::SEN0321() : i2cDevice(DEFAULT_ADDRESS) {
    fTitle = fName = "SEN0321";
}

SEN0321::SEN0321(const char* busAddress, std::uint8_t slaveAddress)
    : i2cDevice(busAddress, slaveAddress) {
    fTitle = fName = "SEN0321";
}

SEN0321::SEN0321(std::uint8_t slaveAddress) : i2cDevice(slaveAddress) {
    fTitle = fName = "SEN0321";
}

SEN0321::~SEN0321() {
}

bool SEN0321::init(MeasureMode mode) {
    if (fMode == MODE_FAILED) {
        return false;
    }
    return setMode(mode);
}

bool SEN0321::setMode(MeasureMode mode) {
    const auto value = static_cast<std::uint8_t>(mode);
    if (!writeByte(static_cast<std::uint8_t>(REG::MODE), value)) {
        return false;
    }
    fModeSetting = mode;
    return true;
}

bool SEN0321::readOzoneRaw(std::uint16_t& ozone) {
    ozone = 0;

    if (fModeSetting == MeasureMode::PASSIVE) {
        std::uint8_t command{0x01};
        if (!writeByte(static_cast<std::uint8_t>(REG::PASSIVE_READ_COMMAND), command)) {
            return false;
        }
    }

    return readWord(static_cast<std::uint8_t>(REG::OZONE_DATA), &ozone);
}

bool SEN0321::getOzonRawValue(std::uint16_t& ozone) {
    return readOzoneRaw(ozone);
}

bool SEN0321::getOzonePpb(double& ozonePpb) {
    std::uint16_t raw{0};
    if (!readOzoneRaw(raw)) {
        ozonePpb = 0.0;
        return false;
    }

    ozonePpb = static_cast<double>(raw);
    return true;
}

bool SEN0321::getOzone(double& ozone) {
    return getOzonePpb(ozone);
}

bool SEN0321::identify() {
    if (fMode == MODE_FAILED) {
        return false;
    }
    if (getAddress() != DEFAULT_ADDRESS) {
        return false;
    }
    return devicePresent();
}

bool SEN0321::devicePresent() {
    if (fMode == MODE_FAILED) {
        return false;
    }

    std::uint8_t modeRegister{0};
    if (readByte(static_cast<std::uint8_t>(REG::MODE), &modeRegister)) {
        return true;
    }

    return i2cDevice::devicePresent();
}
