#include "i2c/qmc5883.h"

#include <cmath>
#include <cstdint>
#include <unistd.h>

/*
 * QMC5883L 3-axis magnetic field sensor.
 *
 * This chip is not register-compatible with the HMC5883L: it uses a different
 * I2C address, little-endian X/Y/Z output registers, and a different control map.
 */

QMC5883::QMC5883() : i2cDevice(DEFAULT_ADDRESS) {
    fTitle = fName = "QMC5883L";
}

QMC5883::QMC5883(const char* busAddress, std::uint8_t slaveAddress)
    : i2cDevice(busAddress, slaveAddress) {
    fTitle = fName = "QMC5883L";
}

QMC5883::QMC5883(std::uint8_t slaveAddress) : i2cDevice(slaveAddress) {
    fTitle = fName = "QMC5883L";
}

QMC5883::~QMC5883() {
}

bool QMC5883::init(MODE mode, OUTPUT_DATA_RATE outputDataRate, RANGE range,
                   OVER_SAMPLE_RATIO overSampleRatio) {
    if (!identify()) {
        return false;
    }
    if (!setResetPeriod()) {
        return false;
    }

    // Disable the interrupt pin by default; polling STATUS.DRDY is enough for this driver.
    std::uint8_t control2 = 0x01;
    if (!writeRegister(REG::CONTROL_2, control2)) {
        return false;
    }

    return setConfig(mode, outputDataRate, range, overSampleRatio);
}

bool QMC5883::reset() {
    std::uint8_t resetCommand = 0x80;
    if (!writeRegister(REG::CONTROL_2, resetCommand)) {
        return false;
    }

    usleep(10000);
    fModeSetting = MODE::STANDBY;
    fOutputDataRate = OUTPUT_DATA_RATE::HZ_10;
    fRange = RANGE::G_2;
    fOverSampleRatio = OVER_SAMPLE_RATIO::OSR_512;
    return true;
}

bool QMC5883::setConfig(MODE mode, OUTPUT_DATA_RATE outputDataRate, RANGE range,
                        OVER_SAMPLE_RATIO overSampleRatio) {
    const auto oldMode = fModeSetting;
    const auto oldOutputDataRate = fOutputDataRate;
    const auto oldRange = fRange;
    const auto oldOverSampleRatio = fOverSampleRatio;

    fModeSetting = mode;
    fOutputDataRate = outputDataRate;
    fRange = range;
    fOverSampleRatio = overSampleRatio;
    if (writeControl1()) {
        return true;
    }

    fModeSetting = oldMode;
    fOutputDataRate = oldOutputDataRate;
    fRange = oldRange;
    fOverSampleRatio = oldOverSampleRatio;
    return false;
}

bool QMC5883::setMode(MODE mode) {
    const auto oldMode = fModeSetting;
    fModeSetting = mode;
    if (writeControl1()) {
        return true;
    }
    fModeSetting = oldMode;
    return false;
}

bool QMC5883::setOutputDataRate(OUTPUT_DATA_RATE outputDataRate) {
    const auto oldOutputDataRate = fOutputDataRate;
    fOutputDataRate = outputDataRate;
    if (writeControl1()) {
        return true;
    }
    fOutputDataRate = oldOutputDataRate;
    return false;
}

bool QMC5883::setRange(RANGE range) {
    const auto oldRange = fRange;
    fRange = range;
    if (writeControl1()) {
        return true;
    }
    fRange = oldRange;
    return false;
}

bool QMC5883::setOverSampleRatio(OVER_SAMPLE_RATIO overSampleRatio) {
    const auto oldOverSampleRatio = fOverSampleRatio;
    fOverSampleRatio = overSampleRatio;
    if (writeControl1()) {
        return true;
    }
    fOverSampleRatio = oldOverSampleRatio;
    return false;
}

bool QMC5883::setResetPeriod(std::uint8_t period) {
    return writeRegister(REG::SET_RESET_PERIOD, period);
}

std::uint8_t QMC5883::chipId() {
    std::uint8_t id{0};
    if (!readByte(static_cast<std::uint8_t>(REG::CHIP_ID), &id)) {
        return 0;
    }
    return id;
}

std::uint8_t QMC5883::status() {
    std::uint8_t value{0};
    if (!readByte(static_cast<std::uint8_t>(REG::STATUS), &value)) {
        return 0;
    }
    return value;
}

bool QMC5883::dataReady() {
    return (status() & 0x01) != 0;
}

bool QMC5883::dataOverflow() {
    return (status() & 0x02) != 0;
}

bool QMC5883::dataSkipped() {
    return (status() & 0x04) != 0;
}

bool QMC5883::waitDataReady(unsigned int timeoutMillis) {
    for (unsigned int elapsedMillis = 0; elapsedMillis <= timeoutMillis; ++elapsedMillis) {
        if (dataReady()) {
            return true;
        }
        usleep(1000);
    }
    return false;
}

QMC5883::RawReading QMC5883::readRaw() {
    RawReading reading{};
    readRaw(reading);
    return reading;
}

bool QMC5883::readRaw(RawReading& reading) {
    if (!waitDataReady() || dataOverflow()) {
        reading = {};
        return false;
    }

    std::uint8_t buf[6]{0};
    const auto n =
        readReg(static_cast<std::uint8_t>(REG::DATA_X_LSB), buf, static_cast<int>(sizeof(buf)));
    if (n != static_cast<int>(sizeof(buf))) {
        reading = {};
        return false;
    }

    reading.x = decodeInt16(buf[0], buf[1]);
    reading.y = decodeInt16(buf[2], buf[3]);
    reading.z = decodeInt16(buf[4], buf[5]);
    reading.valid = true;
    return true;
}

QMC5883::Vector3 QMC5883::readMagneticField() {
    Vector3 magneticField{};
    readMagneticField(magneticField);
    return magneticField;
}

bool QMC5883::readMagneticField(Vector3& magneticFieldGauss) {
    RawReading reading{};
    if (!readRaw(reading)) {
        magneticFieldGauss = {};
        return false;
    }

    const double scale = fullScaleRangeGauss() / 32768.0;
    magneticFieldGauss.x = static_cast<double>(reading.x) * scale;
    magneticFieldGauss.y = static_cast<double>(reading.y) * scale;
    magneticFieldGauss.z = static_cast<double>(reading.z) * scale;
    return true;
}

double QMC5883::readMagnitude() {
    double magnitude{0.0};
    readMagnitude(magnitude);
    return magnitude;
}

bool QMC5883::readMagnitude(double& magnitudeGauss) {
    Vector3 magneticField{};
    if (!readMagneticField(magneticField)) {
        magnitudeGauss = 0.0;
        return false;
    }

    magnitudeGauss =
        std::sqrt(magneticField.x * magneticField.x + magneticField.y * magneticField.y +
                  magneticField.z * magneticField.z);
    return true;
}

bool QMC5883::readTemperatureRaw(std::int16_t& temperature) {
    std::uint8_t buf[2]{0};
    const auto n =
        readReg(static_cast<std::uint8_t>(REG::TEMP_LSB), buf, static_cast<int>(sizeof(buf)));
    if (n != static_cast<int>(sizeof(buf))) {
        temperature = 0;
        return false;
    }

    temperature = decodeInt16(buf[0], buf[1]);
    return true;
}

bool QMC5883::readTemperature(double& temperature) {
    std::int16_t raw{0};
    if (!readTemperatureRaw(raw)) {
        temperature = 0.0;
        return false;
    }

    // Datasheet: the offset is not compensated, only relative temperature is accurate.
    temperature = static_cast<double>(raw) / 100.0;
    return true;
}

bool QMC5883::getMagneticFieldRawValueXYZ(std::int16_t* value) {
    if (value == nullptr) {
        return false;
    }

    RawReading reading{};
    if (!readRaw(reading)) {
        return false;
    }

    value[0] = reading.x;
    value[1] = reading.y;
    value[2] = reading.z;
    return true;
}

bool QMC5883::getMagneticFieldXYZ(double* magnet) {
    if (magnet == nullptr) {
        return false;
    }

    Vector3 magneticField{};
    if (!readMagneticField(magneticField)) {
        return false;
    }

    magnet[0] = magneticField.x;
    magnet[1] = magneticField.y;
    magnet[2] = magneticField.z;
    return true;
}

bool QMC5883::getMagneticField(double& magnet) {
    return readMagnitude(magnet);
}

bool QMC5883::getTemperatureRawValue(std::int16_t& temperature) {
    return readTemperatureRaw(temperature);
}

bool QMC5883::getTemperature(double& temperature) {
    return readTemperature(temperature);
}

double QMC5883::fullScaleRangeGauss() const {
    switch (fRange) {
        case RANGE::G_2:
            return 2.0;
        case RANGE::G_8:
            return 8.0;
    }
    return 2.0;
}

bool QMC5883::identify() {
    if (fMode == i2cDevice::MODE_FAILED) {
        return false;
    }
    return devicePresent();
}

bool QMC5883::devicePresent() {
    if (fMode == i2cDevice::MODE_FAILED) {
        return false;
    }
    return chipId() == EXPECTED_CHIP_ID;
}

bool QMC5883::writeControl1() {
    std::uint8_t value = (static_cast<std::uint8_t>(fOverSampleRatio) << 6) |
                         (static_cast<std::uint8_t>(fRange) << 4) |
                         (static_cast<std::uint8_t>(fOutputDataRate) << 2) |
                         static_cast<std::uint8_t>(fModeSetting);
    return writeRegister(REG::CONTROL_1, value);
}

bool QMC5883::writeRegister(REG reg, std::uint8_t value) {
    return writeReg(static_cast<std::uint8_t>(reg), &value, 1) == 1;
}

std::int16_t QMC5883::decodeInt16(std::uint8_t lowByte, std::uint8_t highByte) {
    return static_cast<std::int16_t>((static_cast<std::uint16_t>(highByte) << 8) | lowByte);
}
