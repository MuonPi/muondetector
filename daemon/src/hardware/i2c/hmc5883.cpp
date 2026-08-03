#include "i2c/hmc5883.h"

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

/*
 * HMC5883 3 axis magnetic field sensor (Honeywell)
 */

const double HMC5883::GAIN[8] = {0.73, 0.92, 1.22, 1.52, 2.27, 2.56, 3.03, 4.35};

HMC5883::HMC5883() : i2cDevice(DEFAULT_ADDRESS) {
    fTitle = fName = "HMC5883";
}

HMC5883::HMC5883(const char* busAddress, uint8_t slaveAddress)
    : i2cDevice(busAddress, slaveAddress) {
    fTitle = fName = "HMC5883";
}

HMC5883::HMC5883(uint8_t slaveAddress) : i2cDevice(slaveAddress) {
    fTitle = fName = "HMC5883";
}

HMC5883::~HMC5883() {
}

bool HMC5883::init() {
    uint8_t readBuf[3]; // 2 byte buffer to store the data read from the I2C device

    // Datasheet default gain setting: GN=001, +/-1.3 Gauss.
    fGain = 1;

    readBuf[0] = 0;
    readBuf[1] = 0;
    readBuf[2] = 0;

    int n =
        readReg(static_cast<uint8_t>(REG::ID_A), readBuf, 3); // Read the id registers into readBuf

    if (fDebugLevel > 1) {
        printf("%d bytes read\n", n);
        printf("id reg A: 0x%x \n", readBuf[0]);
        printf("id reg B: 0x%x \n", readBuf[1]);
        printf("id reg C: 0x%x \n", readBuf[2]);
    }

    if (n != 3 || readBuf[0] != 0x48 || readBuf[1] != 0x34 || readBuf[2] != 0x33)
        return false;

    // addr config reg A (CRA)
    // 8 averages, 15 Hz continuous-rate setting, normal measurement: 0x70
    uint8_t cmd = 0x70;
    n = writeReg(static_cast<uint8_t>(REG::CONFIG_A), &cmd, 1);
    if (n != 1)
        return false;

    return setGain(fGain);
}

bool HMC5883::setGain(uint8_t gain) {
    uint8_t value = (gain & 0x07) << 5;
    if (writeReg(static_cast<uint8_t>(REG::CONFIG_B), &value, 1) != 1)
        return false;

    fGain = gain & 0x07;
    return true;
}

uint8_t HMC5883::readGain() {
    uint8_t readBuf[3]; // 2 byte buffer to store the data read from the I2C device

    int n = readReg(static_cast<uint8_t>(REG::CONFIG_B), readBuf,
                    1); // Read the config register into readBuf

    if (n != 1)
        return 0;
    uint8_t gain = readBuf[0] >> 5;
    if (fDebugLevel > 1) {
        printf("%d bytes read\n", n);
        printf("gain (read from device): 0x%x\n", gain);
    }
    return gain;
}

bool HMC5883::readRDYBit() {
    uint8_t readBuf[3]; // 2 byte buffer to store the data read from the I2C device

    // addr status reg (SR)
    int n = readReg(static_cast<uint8_t>(REG::STATUS), readBuf,
                    1); // Read the status register into readBuf

    if (n != 1)
        return 0;
    uint8_t sr = readBuf[0];
    if (fDebugLevel > 1) {
        printf("%d bytes read\n", n);
        printf("status (read from device): 0x%x\n", sr);
    }
    if ((sr & 0x01) == 0x01)
        return true;
    return false;
}

bool HMC5883::readLockBit() {
    uint8_t readBuf[3]; // 2 byte buffer to store the data read from the I2C device

    // addr status reg (SR)
    int n = readReg(static_cast<uint8_t>(REG::STATUS), readBuf,
                    1); // Read the status register into readBuf

    if (n != 1)
        return 0;
    uint8_t sr = readBuf[0];
    if (fDebugLevel > 1) {
        printf("%d bytes read\n", n);
        printf("status (read from device): 0x%x\n", sr);
    }
    if ((sr & 0x02) == 0x02)
        return true;
    return false;
}

bool HMC5883::waitDataReady(unsigned int timeoutMillis) {
    for (unsigned int elapsedMillis = 0; elapsedMillis <= timeoutMillis; ++elapsedMillis) {
        if (readRDYBit()) {
            return true;
        }
        usleep(1000);
    }
    return false;
}

bool HMC5883::getXYZRawValues(int& x, int& y, int& z) {
    RawReading reading{};
    if (!readRaw(reading)) {
        x = 0;
        y = 0;
        z = 0;
        return false;
    }

    x = reading.x;
    y = reading.y;
    z = reading.z;
    return true;
}

HMC5883::RawReading HMC5883::readRaw() {
    RawReading reading{};
    readRaw(reading);
    return reading;
}

bool HMC5883::readRaw(RawReading& reading) {
    uint8_t readBuf[6]{0};

    uint8_t cmd = 0x01;                                         // start single measurement
    int n = writeReg(static_cast<uint8_t>(REG::MODE), &cmd, 1); // addr mode reg (MR)
    if (n != 1 || !waitDataReady()) {
        reading = {};
        return false;
    }

    // Read the 3 data registers into readBuf starting from addr 0x03
    n = readReg(static_cast<uint8_t>(REG::DATA_X_MSB), readBuf, 6);
    if (n != 6) {
        reading = {};
        return false;
    }

    int16_t xreg = decodeInt16(readBuf[0], readBuf[1]);
    int16_t zreg = decodeInt16(readBuf[2], readBuf[3]);
    int16_t yreg = decodeInt16(readBuf[4], readBuf[5]);

    if (fDebugLevel > 1) {
        printf("%d bytes read\n", n);
        printf("xreg: %d\n", xreg);
        printf("yreg: %d\n", yreg);
        printf("zreg: %d\n", zreg);
    }

    reading.x = xreg;
    reading.y = yreg;
    reading.z = zreg;

    if (xreg >= -2048 && xreg < 2048 && yreg >= -2048 && yreg < 2048 && zreg >= -2048 &&
        zreg < 2048) {
        reading.valid = true;
        return true;
    }

    reading = {};
    return false;
}

HMC5883::Vector3 HMC5883::readMagneticField() {
    Vector3 magneticField{};
    readMagneticField(magneticField);
    return magneticField;
}

bool HMC5883::readMagneticField(Vector3& magneticFieldGauss) {
    RawReading reading{};
    if (!readRaw(reading)) {
        magneticFieldGauss = {};
        return false;
    }

    const double gaussPerLsb = GAIN[fGain] / 1000.;
    magneticFieldGauss.x = gaussPerLsb * reading.x;
    magneticFieldGauss.y = gaussPerLsb * reading.y;
    magneticFieldGauss.z = gaussPerLsb * reading.z;
    return true;
}

bool HMC5883::getXYZMagneticFields(double& x, double& y, double& z) {
    Vector3 magneticField{};
    bool ok = readMagneticField(magneticField);
    x = magneticField.x;
    y = magneticField.y;
    z = magneticField.z;

    if (fDebugLevel > 1) {
        printf("x field: %f G\n", x);
        printf("y field: %f G\n", y);
        printf("z field: %f G\n", z);
    }

    return ok;
}

bool HMC5883::calibrate(int& x, int& y, int& z) {
    // addr config reg A (CRA)
    // 8 average, 15 Hz, positive self test measurement: 0x71
    uint8_t cmd = 0x71;
    if (writeReg(static_cast<uint8_t>(REG::CONFIG_A), &cmd, 1) != 1)
        return false;

    uint8_t oldGain = fGain;
    if (!setGain(5))
        return false;

    int xr, yr, zr;
    // one dummy measurement
    getXYZRawValues(xr, yr, zr);
    // measurement
    getXYZRawValues(xr, yr, zr);

    x = xr;
    y = yr;
    z = zr;

    if (!setGain(oldGain))
        return false;
    // one dummy measurement
    getXYZRawValues(xr, yr, zr);

    // set normal measurement mode in CRA again
    cmd = 0x70;
    return writeReg(static_cast<uint8_t>(REG::CONFIG_A), &cmd, 1) == 1;
}

bool HMC5883::identify() {
    if (fMode == MODE_FAILED) {
        return false;
    }
    return devicePresent();
}

bool HMC5883::devicePresent() {
    if (fMode == MODE_FAILED) {
        return false;
    }

    uint8_t readBuf[3]{0};
    return readReg(static_cast<uint8_t>(REG::ID_A), readBuf, 3) == 3 && readBuf[0] == 0x48 &&
           readBuf[1] == 0x34 && readBuf[2] == 0x33;
}

std::int16_t HMC5883::decodeInt16(std::uint8_t highByte, std::uint8_t lowByte) {
    return static_cast<std::int16_t>((static_cast<std::uint16_t>(highByte) << 8) | lowByte);
}
