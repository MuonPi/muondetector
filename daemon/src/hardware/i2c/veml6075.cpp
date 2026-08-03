#include "i2c/veml6075.h"

#include <algorithm>
#include <cmath>

VEML6075::VEML6075() : i2cDevice(DEFAULT_ADDRESS) {
    fTitle = fName = "VEML6075";
}

VEML6075::VEML6075(const char* busAddress, std::uint8_t slaveAddress)
    : i2cDevice(busAddress, slaveAddress) {
    fTitle = fName = "VEML6075";
}

VEML6075::VEML6075(std::uint8_t slaveAddress) : i2cDevice(slaveAddress) {
    fTitle = fName = "VEML6075";
}

VEML6075::~VEML6075() {
}

bool VEML6075::init() {
    if (!devicePresent()) {
        return false;
    }
    return setConfig();
}

bool VEML6075::setConfig() {
    auto config = configByte();
    return writeByte(static_cast<std::uint8_t>(REG::CONF), config);
}

bool VEML6075::readRaw(RawReading& reading) {
    reading = {};

    if (!readWordLittleEndian(REG::UVA_DATA, reading.uva)) {
        return false;
    }
    if (!readWordLittleEndian(REG::UVB_DATA, reading.uvb)) {
        return false;
    }
    if (!readWordLittleEndian(REG::UVCOMP1_DATA, reading.uvComp1)) {
        return false;
    }
    if (!readWordLittleEndian(REG::UVCOMP2_DATA, reading.uvComp2)) {
        return false;
    }

    reading.valid = true;
    return true;
}

VEML6075::UVReading VEML6075::readUV() {
    UVReading reading{};
    readUV(reading);
    return reading;
}

bool VEML6075::readUV(UVReading& reading) {
    reading = {};
    RawReading raw{};
    if (!readRaw(raw)) {
        return false;
    }

    const double uvaCompensated = static_cast<double>(raw.uva) -
                                  (UVA_VIS_COEFF * static_cast<double>(raw.uvComp1)) -
                                  (UVA_IR_COEFF * static_cast<double>(raw.uvComp2));
    const double uvbCompensated = static_cast<double>(raw.uvb) -
                                  (UVB_VIS_COEFF * static_cast<double>(raw.uvComp1)) -
                                  (UVB_IR_COEFF * static_cast<double>(raw.uvComp2));

    reading.raw = raw;
    reading.uvaIndex = std::max(0.0, uvaCompensated * UVA_RESPONSIVITY);
    reading.uvbIndex = std::max(0.0, uvbCompensated * UVB_RESPONSIVITY);
    reading.uvIndex = (reading.uvaIndex + reading.uvbIndex) / 2.0;
    reading.valid = true;
    return true;
}

bool VEML6075::getUVRawValue(std::int16_t* value) {
    if (value == nullptr) {
        return false;
    }

    RawReading reading{};
    if (!readRaw(reading)) {
        return false;
    }

    value[0] = static_cast<std::int16_t>(reading.uva);
    value[1] = static_cast<std::int16_t>(reading.uvb);
    value[2] = static_cast<std::int16_t>(reading.uvComp1);
    value[3] = static_cast<std::int16_t>(reading.uvComp2);
    return true;
}

bool VEML6075::getUV(double* uv) {
    if (uv == nullptr) {
        return false;
    }

    UVReading reading{};
    if (!readUV(reading)) {
        return false;
    }

    uv[0] = reading.uvaIndex;
    uv[1] = reading.uvbIndex;
    uv[2] = static_cast<double>(reading.raw.uva);
    uv[3] = static_cast<double>(reading.raw.uvb);
    return true;
}

bool VEML6075::identify() {
    if (fMode == MODE_FAILED) {
        return false;
    }
    return devicePresent();
}

bool VEML6075::devicePresent() {
    if (fMode == MODE_FAILED) {
        return false;
    }
    return deviceId() == EXPECTED_DEVICE_ID;
}

std::uint16_t VEML6075::deviceId() {
    std::uint16_t id{0};
    if (!readWordLittleEndian(REG::ID, id)) {
        return 0;
    }
    return id;
}

bool VEML6075::readWordLittleEndian(REG reg, std::uint16_t& value) {
    std::uint8_t buf[2]{0};
    if (readReg(static_cast<std::uint8_t>(reg), buf, static_cast<int>(sizeof(buf))) !=
        static_cast<int>(sizeof(buf))) {
        value = 0;
        return false;
    }

    value = static_cast<std::uint16_t>(buf[0]);
    value |= static_cast<std::uint16_t>(buf[1]) << 8;
    return true;
}

std::uint8_t VEML6075::configByte() const {
    std::uint8_t config{0};
    config |= static_cast<std::uint8_t>((UV_IT & 0x07) << 4);
    config |= static_cast<std::uint8_t>((HD & 0x01) << 3);
    config |= static_cast<std::uint8_t>((UV_TRIG & 0x01) << 2);
    config |= static_cast<std::uint8_t>((UV_AF & 0x01) << 1);
    config |= static_cast<std::uint8_t>(AD & 0x01);
    return config;
}
