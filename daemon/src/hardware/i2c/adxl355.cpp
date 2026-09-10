#include "i2c/adxl355.h"

#include <array>
#include <iostream>

const std::unordered_map<ADXL355::REG, std::uint8_t>
    ADXL355::registerMap{
        {REG::DEVID_AD,  0x00},
        {REG::DEVID_MST, 0x01},
        {REG::PARTID,    0x02},

        {REG::TEMP2,     0x06},
        {REG::TEMP1,     0x07},

        {REG::XDATA3,    0x08},
        {REG::XDATA2,    0x09},
        {REG::XDATA1,    0x0A},

        {REG::YDATA3,    0x0B},
        {REG::YDATA2,    0x0C},
        {REG::YDATA1,    0x0D},

        {REG::ZDATA3,    0x0E},
        {REG::ZDATA2,    0x0F},
        {REG::ZDATA1,    0x10},

        {REG::FILTER,    0x28},

        {REG::RANGE,     0x2C},
        {REG::POWER_CTL, 0x2D},
        {REG::RESET,     0x2F}
    };

const std::unordered_map<ADXL355::RANGE, std::uint8_t>
    ADXL355::rangeValueMap{
        {RANGE::_2G, 0b01},
        {RANGE::_4G, 0b10},
        {RANGE::_8G, 0b11}
    };

/*
 * Sensitivity in LSB/g.
 *
 * ADXL355:
 *   +/- 2 g -> 256000 LSB/g
 *   +/- 4 g -> 128000 LSB/g
 *   +/- 8 g ->  64000 LSB/g
 */
const std::unordered_map<ADXL355::RANGE, double>
    ADXL355::sensitivityMap{
        {RANGE::_2G, 256000.0},
        {RANGE::_4G, 128000.0},
        {RANGE::_8G,  64000.0}
    };

const std::unordered_map<ADXL355::ODR, std::uint8_t>
    ADXL355::odrValueMap{
        {ODR::_4000HZ,    0x00},
        {ODR::_2000HZ,    0x01},
        {ODR::_1000HZ,    0x02},
        {ODR::_500HZ,     0x03},
        {ODR::_250HZ,     0x04},
        {ODR::_125HZ,     0x05},
        {ODR::_62_5HZ,    0x06},
        {ODR::_31_25HZ,   0x07},
        {ODR::_15_625HZ,  0x08},
        {ODR::_7_813HZ,   0x09},
        {ODR::_3_906HZ,   0x0A}
    };

const std::unordered_map<ADXL355::HPF, std::uint8_t>
    ADXL355::hpfValueMap{
        {HPF::OFF,      0x00},
        {HPF::_247E_3,  0x01},
        {HPF::_62E_3,   0x02},
        {HPF::_15E_3,   0x03},
        {HPF::_3E_3,    0x04},
        {HPF::_0_95E_3, 0x05},
        {HPF::_0_24E_3, 0x06}
    };

const std::unordered_map<ADXL355::MODE, std::uint8_t>
    ADXL355::modeValueMap{
        {MODE::STANDBY,     0x01},
        {MODE::MEASUREMENT, 0x00}
    };




ADXL355::ADXL355()
    : i2cDevice(0x1D) {
    fTitle = "ADXL355";
}

ADXL355::ADXL355(const char* busAddress, std::uint8_t slaveAddress)
    : i2cDevice(busAddress, slaveAddress) {
    fTitle = "ADXL355";
}

ADXL355::ADXL355(std::uint8_t slaveAddress)
    : i2cDevice(slaveAddress) {
    fTitle = "ADXL355";
}

ADXL355::~ADXL355() {
}


bool ADXL355::init() {
    if (!reset()) {
        return false;
    }

    if (!identify()) {
        return false;
    }

    // setRange(RANGE::_2G);

    std::uint8_t power = 0x00;

    if (writeReg(registerMap.at(REG::POWER_CTL), &power, 1) < 0) {
        return false;
    }

    return true;
}


bool ADXL355::identifyDevice(std::uint8_t address) {
    ADXL355 device(address);
    return device.identify();
}


bool ADXL355::identify() {
    if (fMode == MODE_FAILED) {
        return false;
    }

    std::array<std::uint8_t, 3> buf{};

    if (readReg(registerMap.at(REG::DEVID_AD), buf.data(), 3) < 0) {
        return false;
    }

    return buf.at(0) == 0xAD &&
           buf.at(1) == 0x1D &&
           buf.at(2) == 0xED;
}


bool ADXL355::reset() {
    std::uint8_t value = 0x52;

    /*
     * Writing 0x52 to RESET performs a software reset.
     */
    return !(writeReg(registerMap.at(REG::RESET), &value, 1) < 0);
}


bool ADXL355::setMode(MODE mode) {

    std::uint8_t value{};

    if (readReg(registerMap.at(REG::POWER_CTL), &value, 1) < 0) {
        return false;
    }

    value &= 0xFE;
    value |= modeValueMap.at(mode);

    if (writeReg(registerMap.at(REG::POWER_CTL), &value, 1) < 0) {
        return false;
    }

    currentMode = mode;

    return true;
}


void ADXL355::setRange(RANGE range) {
    std::uint8_t value{};

    if (readReg(registerMap.at(REG::RANGE), &value, 1) < 0) {
        return;
    }

    /*
     * RANGE register:
     *
     * bits 7:2 = reserved
     * bits 1:0 = range
     *
     * Keep the reserved bits unchanged.
     */
    value &= 0xFC;
    value |= rangeValueMap.at(range);

    if (writeReg(registerMap.at(REG::RANGE), &value, 1) < 0) {
        return;
    }

    currentRange = range;
}


ADXL355::RANGE ADXL355::getRange() const {
    return currentRange;
}


std::optional<std::int32_t>
ADXL355::readAxis(REG reg) {
    std::array<std::uint8_t, 3> buf{};

    if (readReg(registerMap.at(reg), buf.data(), 3) < 0) {
        return std::nullopt;
    }

    /*
     * ADXL355 acceleration data is 20-bit two's complement.
     *
     * Register order:
     *
     * DATA3 DATA2 DATA1
     *
     * The 20-bit value is:
     *
     *     DATA3 << 12
     *   | DATA2 << 4
     *   | DATA1 >> 4
     */

    std::int32_t value =
        (static_cast<std::int32_t>(buf.at(0)) << 12) |
        (static_cast<std::int32_t>(buf.at(1)) << 4)  |
        (static_cast<std::int32_t>(buf.at(2)) >> 4);

    /*
     * Sign extend 20-bit two's complement to 32 bit.
     */
    if (value & (1 << 19)) {
        value |= ~((1 << 20) - 1);
    }

    // std::cout << "RAW: "
    //       << std::hex
    //       << static_cast<int>(buf[0]) << " "
    //       << static_cast<int>(buf[1]) << " "
    //       << static_cast<int>(buf[2])
    //       << std::dec << std::endl;

    return value;
}


std::optional<double> ADXL355::readX() {
    auto value = readAxis(REG::XDATA3);

    if (!value.has_value()) {
        return std::nullopt;
    }

    return static_cast<double>(value.value()) /
           sensitivityMap.at(currentRange);
}


std::optional<double> ADXL355::readY() {
    auto value = readAxis(REG::YDATA3);

    if (!value.has_value()) {
        return std::nullopt;
    }

    return static_cast<double>(value.value()) /
           sensitivityMap.at(currentRange);
}


std::optional<double> ADXL355::readZ() {
    auto value = readAxis(REG::ZDATA3);

    if (!value.has_value()) {
        return std::nullopt;
    }

    return static_cast<double>(value.value()) /
           sensitivityMap.at(currentRange);
}


std::optional<ADXL355::Acceleration>
ADXL355::readAcceleration() {

    auto x = readX();
    auto y = readY();
    auto z = readZ();

    if (!x.has_value() ||
        !y.has_value() ||
        !z.has_value()) {
        return std::nullopt;
    }

    return Acceleration{
        .x = x.value(),
        .y = y.value(),
        .z = z.value()
    };
}


bool ADXL355::setODR(ODR odr) {

    std::uint8_t value{};

    if (readReg(registerMap.at(REG::FILTER), &value, 1) < 0) {
        return false;
    }

    value &= 0xF0;
    value |= odrValueMap.at(odr);

    if (writeReg(registerMap.at(REG::FILTER), &value, 1) < 0) {
        return false;
    }

    currentODR = odr;

    return true;
}


ADXL355::ODR ADXL355::getODR() const {
    return currentODR;
}


bool ADXL355::setHPF(HPF hpf) {

    std::uint8_t value{};

    if (readReg(registerMap.at(REG::FILTER), &value, 1) < 0) {
        return false;
    }

    value &= 0x0F;
    value |= static_cast<std::uint8_t>(hpfValueMap.at(hpf) << 4);

    if (writeReg(registerMap.at(REG::FILTER), &value, 1) < 0) {
        return false;
    }

    currentHPF = hpf;

    return true;
}


ADXL355::HPF ADXL355::getHPF() const {
    return currentHPF;
}


std::optional<double> ADXL355::readTemperature()
{
    std::uint8_t temp2{};
    std::uint8_t temp1{};

    if (readReg(registerMap.at(REG::TEMP2), &temp2, 1) < 0) {
        return std::nullopt;
    }

    if (readReg(registerMap.at(REG::TEMP1), &temp1, 1) < 0) {
        return std::nullopt;
    }

    std::uint16_t raw =
        (static_cast<std::uint16_t>(temp2) << 8) |
        static_cast<std::uint16_t>(temp1);

    return 25.0 - (static_cast<double>(raw) - 1885.0) / 9.05;
}
