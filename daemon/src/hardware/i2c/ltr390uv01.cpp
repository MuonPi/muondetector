#include "i2c/ltr390uv01.h"

#include <cstdint>
#include <unordered_map>

const std::unordered_map<LTR390UV01::REG, std::uint8_t> LTR390UV01::registerMap{
    {REG::MAIN_CTRL, 0x00},   {REG::MEAS_CONFIG, 0x04}, {REG::GAIN, 0x05},    {REG::PART_ID, 0x06},
    {REG::MAIN_STATUS, 0x07}, {REG::ALSDATA, 0x0D},     {REG::UVSDATA, 0x10}, {REG::INT_CFG, 0x19}};

const std::unordered_map<LTR390UV01::GAIN, std::uint8_t> LTR390UV01::gainSetting{
    {LTR390UV01::GAIN::_1, 0x00}, {LTR390UV01::GAIN::_3, 0x01},  {LTR390UV01::GAIN::_6, 0x02},
    {LTR390UV01::GAIN::_9, 0x03}, {LTR390UV01::GAIN::_18, 0x04},
};

const std::unordered_map<LTR390UV01::RESOLUTION, std::uint8_t> LTR390UV01::resolutionSetting{
    {LTR390UV01::RESOLUTION::_20bit, 0x00}, {LTR390UV01::RESOLUTION::_19bit, 0x01},
    {LTR390UV01::RESOLUTION::_18bit, 0x02}, {LTR390UV01::RESOLUTION::_17bit, 0x03},
    {LTR390UV01::RESOLUTION::_16bit, 0x04}, {LTR390UV01::RESOLUTION::_13bit, 0x05},
};

const std::unordered_map<LTR390UV01::INTERVAL, std::uint8_t> LTR390UV01::intervalSetting{
    {LTR390UV01::INTERVAL::_25ms, 0x00},   {LTR390UV01::INTERVAL::_50ms, 0x01},
    {LTR390UV01::INTERVAL::_100ms, 0x02},  {LTR390UV01::INTERVAL::_200ms, 0x03},
    {LTR390UV01::INTERVAL::_500ms, 0x04},  {LTR390UV01::INTERVAL::_1000ms, 0x05},
    {LTR390UV01::INTERVAL::_2000ms, 0x06},
};

const std::unordered_map<LTR390UV01::GAIN, double> LTR390UV01::gainCorrection{
    {LTR390UV01::GAIN::_1, 1.0}, {LTR390UV01::GAIN::_3, 3.0},   {LTR390UV01::GAIN::_6, 6.0},
    {LTR390UV01::GAIN::_9, 9.0}, {LTR390UV01::GAIN::_18, 18.0},
};

const std::unordered_map<LTR390UV01::RESOLUTION, double> LTR390UV01::resolutionCorrection{
    {LTR390UV01::RESOLUTION::_13bit, 0.125}, {LTR390UV01::RESOLUTION::_16bit, 0.25},
    {LTR390UV01::RESOLUTION::_17bit, 0.5},   {LTR390UV01::RESOLUTION::_18bit, 1.0},
    {LTR390UV01::RESOLUTION::_19bit, 2.0},   {LTR390UV01::RESOLUTION::_20bit, 4.0}};
/*
 * LTR390UV01 UV Sensor
 */
LTR390UV01::LTR390UV01() : i2cDevice(0x53) {
    fTitle = fName = "LTR390UV01";
    init();
}

LTR390UV01::LTR390UV01(const char* busAddress, uint8_t slaveAddress)
    : i2cDevice(busAddress, slaveAddress) {
    fTitle = fName = "LTR390UV01";
    init();
}

LTR390UV01::LTR390UV01(uint8_t slaveAddress) : i2cDevice(slaveAddress) {
    fTitle = fName = "LTR390UV01";
    init();
}

LTR390UV01::~LTR390UV01() {
}

bool LTR390UV01::identify() {
    if (fMode == MODE_FAILED) {
        return false;
    }
    return devicePresent();
}

bool LTR390UV01::devicePresent() {
    std::uint8_t deviceId{0};
    return readReg(registerMap.at(REG::PART_ID), &deviceId, 1) == 1;
}

void LTR390UV01::init() {
    /*
     * LTR-390 Control Register
     * ------------------------
     *
     * Bit layout:
     *
     *  Bits   Name        Default   Description
     * ------------------------------------------------------------
     *  7:5    Reserved      000      Reserved
     *
     *   4     SW Reset       0       Software Reset:
     *                                0 = not triggered (default)
     *                                1 = trigger software reset
     *
     *   3     UVS_Mode       0       Mode Select:
     *                                0 = ALS mode
     *                                1 = UVS mode
     *
     *   2     Reserved       0       Reserved
     *
     *   1     ALS/UVS        0       Sensor selection:
     *                                0 = standby
     *                                1 = active
     *
     *   0     Enable         0       Light sensor enable:
     *                                0 = standby
     *                                1 = active
     *
     * Notes:
     * - Reserved bits must always be written as 0
     * - Default state is all bits cleared (0x00)
     */
    // Reset
    std::uint8_t reset = 0x10;
    writeReg(registerMap.at(REG::MAIN_CTRL), &reset, 1);

    usleep(10000); // 10ms

    std::uint8_t mainCtrlRegValue{0b1010}; // UVS Mode + Sensor Active
    currentMode = MODE::UVS;
    writeReg(registerMap.at(REG::MAIN_CTRL), &mainCtrlRegValue, 1);
    setInterval(INTERVAL::_500ms);
    setResolution(RESOLUTION::_20bit);
    setGain(GAIN::_18);
}

auto LTR390UV01::id() -> std::uint8_t {
    std::uint8_t buf;
    readReg(registerMap.at(REG::PART_ID), &buf, 1);
    return buf;
}

auto LTR390UV01::mainStatus() -> Status {
    std::uint8_t buf{0};
    if (readReg(registerMap.at(REG::MAIN_STATUS), &buf, 1) != 1) {
        return {};
    }
    return Status{.powerOnStatus = ((buf >> 5) & 0x01) == 0x01,
                  .interruptStatus = ((buf >> 4) & 0x01) == 0x01,
                  .dataStatus = ((buf >> 3) & 0x01) == 0x01};
}

void LTR390UV01::setResolution(RESOLUTION resolution) {
    currentResolution = resolution;
    meas_and_rate_register =
        (meas_and_rate_register & 0x0f) | (resolutionSetting.at(resolution) << 4);
    writeReg(registerMap.at(REG::MEAS_CONFIG), &meas_and_rate_register, 1);
}

void LTR390UV01::setInterval(INTERVAL interval) {
    currentInterval = interval;
    meas_and_rate_register = (meas_and_rate_register & 0xf0) | intervalSetting.at(interval);
    writeReg(registerMap.at(REG::MEAS_CONFIG), &meas_and_rate_register, 1);
}

void LTR390UV01::setInterrupt(MODE mode, bool isEnabled) {
    std::uint8_t regValue =
        ((mode == MODE::ALS ? 0x01 : 0x10) << 4) | ((isEnabled ? 0x01 : 0x00) << 2);
    writeReg(registerMap.at(REG::INT_CFG), &regValue, 1);
}

void LTR390UV01::setGain(GAIN gain) {
    currentGain = gain;
    auto gainValue = gainSetting.at(gain);
    writeReg(registerMap.at(REG::GAIN), &gainValue, 1);
}

auto LTR390UV01::readUVS() -> std::uint32_t {
    std::uint8_t buf[3]{0};
    auto result = readReg(registerMap.at(REG::UVSDATA), buf, 3);
    if (result != 3) {
        std::cerr << "Reading LTR390UV01 UVS returned -1";
        return 0;
    }
    std::uint32_t data{0x00};
    data |= static_cast<std::uint32_t>(buf[2] & 0x0f);
    data = data << 8;
    data |= static_cast<std::uint32_t>(buf[1]);
    data = data << 8;
    data |= static_cast<std::uint32_t>(buf[0]);
    return data;
}

auto LTR390UV01::readALS() -> std::uint32_t {
    std::uint8_t buf[3]{0};
    auto result = readReg(registerMap.at(REG::ALSDATA), buf, 3);
    if (result != 3) {
        std::cerr << "Reading LTR390UV01 ALS returned -1";
        return 0;
    }
    std::uint32_t data{0x00};
    data |= static_cast<std::uint32_t>(buf[2] & 0x0f);
    data = data << 8;
    data |= static_cast<std::uint32_t>(buf[1]);
    data = data << 8;
    data |= static_cast<std::uint32_t>(buf[0]);
    return data;
}

auto LTR390UV01::read() -> double {
    std::uint32_t value;
    auto gain = gainCorrection.at(currentGain);
    auto integrationTime = resolutionCorrection.at(currentResolution);
    if (currentMode == MODE::UVS) {
        value = readUVS();
        return value / 1400. * resolutionCorrection.at(RESOLUTION::_20bit) /
               integrationTime                        // Reference specifies resolution 20bit
               * gainCorrection.at(GAIN::_18) / gain; // Reference specifies gain 18
    } else {
        value = readALS();
        return 0.6 * value / (gain * integrationTime);
    }
}
