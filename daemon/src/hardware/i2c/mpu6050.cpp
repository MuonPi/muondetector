#include "i2c/mpu6050.h"

#include <stdint.h>
#include <unistd.h>

/*
 * MPU6050 6-axis accelerometer/gyroscope.
 * GY-521 breakout boards usually expose this chip at 0x68, or 0x69 when AD0 is high.
 */

MPU6050::MPU6050() : i2cDevice(DEFAULT_ADDRESS) {
    fTitle = fName = "MPU6050/GY-521";
}

MPU6050::MPU6050(const char* busAddress, std::uint8_t slaveAddress)
    : i2cDevice(busAddress, slaveAddress) {
    fTitle = fName = "MPU6050/GY-521";
}

MPU6050::MPU6050(std::uint8_t slaveAddress) : i2cDevice(slaveAddress) {
    fTitle = fName = "MPU6050/GY-521";
}

MPU6050::~MPU6050() {
}

bool MPU6050::init(GYRO_RANGE gyroRange, ACCEL_RANGE accelRange, DLPF dlpf,
                   std::uint8_t sampleRateDivider) {
    if (!identify()) {
        return false;
    }
    if (!wake()) {
        return false;
    }

    std::uint8_t allAxesEnabled{0x00};
    if (!writeByte(static_cast<std::uint8_t>(REG::PWR_MGMT_2), allAxesEnabled)) {
        return false;
    }
    if (!setDigitalLowPassFilter(dlpf)) {
        return false;
    }
    if (!setSampleRateDivider(sampleRateDivider)) {
        return false;
    }
    if (!setGyroRange(gyroRange)) {
        return false;
    }
    if (!setAccelRange(accelRange)) {
        return false;
    }
    return true;
}

bool MPU6050::reset() {
    std::uint8_t resetCommand{0x80};
    if (!writeByte(static_cast<std::uint8_t>(REG::PWR_MGMT_1), resetCommand)) {
        return false;
    }
    usleep(100000);

    fGyroRange = GYRO_RANGE::DPS_250;
    fAccelRange = ACCEL_RANGE::G_2;
    fDlpf = DLPF::ACCEL_260HZ_GYRO_256HZ;
    fSampleRateDivider = 0;
    return true;
}

bool MPU6050::wake(CLOCK_SOURCE clockSource) {
    std::uint8_t powerManagement =
        static_cast<std::uint8_t>(clockSource) & static_cast<std::uint8_t>(0x07);
    return writeByte(static_cast<std::uint8_t>(REG::PWR_MGMT_1), powerManagement);
}

bool MPU6050::sleep(bool enabled) {
    return updateRegister(REG::PWR_MGMT_1, 0x40, enabled ? 0x40 : 0x00);
}

bool MPU6050::setSampleRateDivider(std::uint8_t divider) {
    if (!writeByte(static_cast<std::uint8_t>(REG::SMPLRT_DIV), divider)) {
        return false;
    }
    fSampleRateDivider = divider;
    return true;
}

bool MPU6050::setDigitalLowPassFilter(DLPF dlpf) {
    if (!updateRegister(REG::CONFIG, 0x07, static_cast<std::uint8_t>(dlpf))) {
        return false;
    }
    fDlpf = dlpf;
    return true;
}

bool MPU6050::setGyroRange(GYRO_RANGE range) {
    const auto value = static_cast<std::uint8_t>(static_cast<std::uint8_t>(range) << 3);
    if (!updateRegister(REG::GYRO_CONFIG, 0x18, value)) {
        return false;
    }
    fGyroRange = range;
    return true;
}

bool MPU6050::setAccelRange(ACCEL_RANGE range) {
    const auto value = static_cast<std::uint8_t>(static_cast<std::uint8_t>(range) << 3);
    if (!updateRegister(REG::ACCEL_CONFIG, 0x18, value)) {
        return false;
    }
    fAccelRange = range;
    return true;
}

bool MPU6050::readGyroRange(GYRO_RANGE& range) {
    std::uint8_t value{0};
    if (!readByte(static_cast<std::uint8_t>(REG::GYRO_CONFIG), &value)) {
        return false;
    }
    range = static_cast<GYRO_RANGE>((value >> 3) & 0x03);
    fGyroRange = range;
    return true;
}

bool MPU6050::readAccelRange(ACCEL_RANGE& range) {
    std::uint8_t value{0};
    if (!readByte(static_cast<std::uint8_t>(REG::ACCEL_CONFIG), &value)) {
        return false;
    }
    range = static_cast<ACCEL_RANGE>((value >> 3) & 0x03);
    fAccelRange = range;
    return true;
}

std::uint8_t MPU6050::whoAmI() {
    std::uint8_t deviceId{0};
    if (!readByte(static_cast<std::uint8_t>(REG::WHO_AM_I), &deviceId)) {
        return 0;
    }
    return deviceId;
}

bool MPU6050::identify() {
    if (fMode == MODE_FAILED) {
        return false;
    }
    return devicePresent();
}

bool MPU6050::devicePresent() {
    if (fMode == MODE_FAILED) {
        return false;
    }
    return isExpectedWhoAmI(whoAmI());
}

MPU6050::RawReading MPU6050::readRaw() {
    RawReading reading{};
    readRaw(reading);
    return reading;
}

bool MPU6050::readRaw(RawReading& reading) {
    std::uint8_t buf[14]{0};

    startTimer();
    const auto n =
        readReg(static_cast<std::uint8_t>(REG::ACCEL_XOUT_H), buf, static_cast<int>(sizeof(buf)));
    stopTimer();

    if (n != static_cast<int>(sizeof(buf))) {
        reading = {};
        return false;
    }

    reading.accelX = decodeInt16(buf[0], buf[1]);
    reading.accelY = decodeInt16(buf[2], buf[3]);
    reading.accelZ = decodeInt16(buf[4], buf[5]);
    reading.temperature = decodeInt16(buf[6], buf[7]);
    reading.gyroX = decodeInt16(buf[8], buf[9]);
    reading.gyroY = decodeInt16(buf[10], buf[11]);
    reading.gyroZ = decodeInt16(buf[12], buf[13]);
    reading.valid = true;

    fLastRawReading = reading;
    return true;
}

MPU6050::Measurement MPU6050::getMeasurement() {
    Measurement measurement{};
    getMeasurement(measurement);
    return measurement;
}

bool MPU6050::getMeasurement(Measurement& measurement) {
    RawReading raw{};
    if (!readRaw(raw)) {
        measurement = {};
        return false;
    }

    measurement.accelerationG = scaleAcceleration(raw);
    measurement.gyroscopeDps = scaleGyroscope(raw);
    measurement.temperatureC = static_cast<double>(raw.temperature) / 340.0 + 36.53;
    measurement.valid = true;

    fLastMeasurement = measurement;
    return true;
}

bool MPU6050::getAcceleration(Vector3& accelerationG) {
    return readScaledVector(REG::ACCEL_XOUT_H, accelerationSensitivity(), accelerationG);
}

bool MPU6050::getGyroscope(Vector3& gyroscopeDps) {
    return readScaledVector(REG::GYRO_XOUT_H, gyroscopeSensitivity(), gyroscopeDps);
}

bool MPU6050::getTemperature(double& temperatureC) {
    std::uint8_t buf[2]{0};
    const auto n =
        readReg(static_cast<std::uint8_t>(REG::TEMP_OUT_H), buf, static_cast<int>(sizeof(buf)));
    if (n != static_cast<int>(sizeof(buf))) {
        temperatureC = -999.0;
        return false;
    }

    temperatureC = static_cast<double>(decodeInt16(buf[0], buf[1])) / 340.0 + 36.53;
    return true;
}

double MPU6050::getTemperature() {
    double temperature{-999.0};
    getTemperature(temperature);
    return temperature;
}

double MPU6050::accelerationSensitivity() const {
    switch (fAccelRange) {
        case ACCEL_RANGE::G_2:
            return 16384.0;
        case ACCEL_RANGE::G_4:
            return 8192.0;
        case ACCEL_RANGE::G_8:
            return 4096.0;
        case ACCEL_RANGE::G_16:
            return 2048.0;
    }
    return 16384.0;
}

double MPU6050::gyroscopeSensitivity() const {
    switch (fGyroRange) {
        case GYRO_RANGE::DPS_250:
            return 131.0;
        case GYRO_RANGE::DPS_500:
            return 65.5;
        case GYRO_RANGE::DPS_1000:
            return 32.8;
        case GYRO_RANGE::DPS_2000:
            return 16.4;
    }
    return 131.0;
}

bool MPU6050::isValidAddress(std::uint8_t address) {
    return address == DEFAULT_ADDRESS || address == ALTERNATE_ADDRESS;
}

bool MPU6050::isExpectedWhoAmI(std::uint8_t deviceId) {
    return (deviceId & 0x7e) == EXPECTED_WHO_AM_I;
}

bool MPU6050::updateRegister(REG reg, std::uint8_t mask, std::uint8_t value) {
    std::uint8_t regValue{0};
    if (!readByte(static_cast<std::uint8_t>(reg), &regValue)) {
        return false;
    }

    regValue =
        static_cast<std::uint8_t>((regValue & static_cast<std::uint8_t>(~mask)) | (value & mask));
    return writeByte(static_cast<std::uint8_t>(reg), regValue);
}

bool MPU6050::readScaledVector(REG reg, double sensitivity, Vector3& vector) {
    std::uint8_t buf[6]{0};
    const auto n = readReg(static_cast<std::uint8_t>(reg), buf, static_cast<int>(sizeof(buf)));
    if (n != static_cast<int>(sizeof(buf))) {
        vector = {};
        return false;
    }

    vector.x = static_cast<double>(decodeInt16(buf[0], buf[1])) / sensitivity;
    vector.y = static_cast<double>(decodeInt16(buf[2], buf[3])) / sensitivity;
    vector.z = static_cast<double>(decodeInt16(buf[4], buf[5])) / sensitivity;
    return true;
}

MPU6050::Vector3 MPU6050::scaleAcceleration(const RawReading& reading) const {
    const auto sensitivity = accelerationSensitivity();
    return {.x = static_cast<double>(reading.accelX) / sensitivity,
            .y = static_cast<double>(reading.accelY) / sensitivity,
            .z = static_cast<double>(reading.accelZ) / sensitivity};
}

MPU6050::Vector3 MPU6050::scaleGyroscope(const RawReading& reading) const {
    const auto sensitivity = gyroscopeSensitivity();
    return {.x = static_cast<double>(reading.gyroX) / sensitivity,
            .y = static_cast<double>(reading.gyroY) / sensitivity,
            .z = static_cast<double>(reading.gyroZ) / sensitivity};
}

std::int16_t MPU6050::decodeInt16(std::uint8_t highByte, std::uint8_t lowByte) {
    return static_cast<std::int16_t>((static_cast<std::uint16_t>(highByte) << 8) | lowByte);
}
