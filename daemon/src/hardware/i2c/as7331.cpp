#include "hardware/i2c/as7331.h"

#include <iomanip>
#include <iostream>
#include <sstream>

/*
The 4 MSB bits of the register address are ignored.
OUTCONV is only available in SYND measurement mode with bit CREG2:EN_TM = “1”. The least significant
byte comes first.
*/
const std::unordered_map<AS7331::REG, std::uint8_t> AS7331::registerMap{
    {REG::OSR_STATUS, 0x00}, {REG::TEMP, 0x01},   {REG::AGEN, 0x02},      {REG::MRES_A, 0x02},
    {REG::MRES_B, 0x03},     {REG::MRES_C, 0x04}, {REG::OUTCONV_L, 0x05}, {REG::OUTCONV_H, 0x06},
    {REG::CREG_1, 0x06},     {REG::CREG_2, 0x07}, {REG::CREG_3, 0x08},    {REG::BREAK, 0x09},
    {REG::EDGES, 0x0A},      {REG::OPTREG, 0x0B}};

const std::unordered_map<AS7331::GAIN, std::uint8_t> AS7331::gainValueMap{
    {GAIN::_2048x, 0b0000}, {GAIN::_1024x, 0b0001}, {GAIN::_512x, 0b0010}, {GAIN::_256x, 0b0011},
    {GAIN::_128x, 0b0100},  {GAIN::_64x, 0b0101},   {GAIN::_32x, 0b0110},  {GAIN::_16x, 0b0111},
    {GAIN::_8x, 0b1000},    {GAIN::_4x, 0b1001},    {GAIN::_2x, 0b1010},   {GAIN::_1x, 0b1011},
};

const std::unordered_map<AS7331::GAIN, double> AS7331::fullScaleRangeMapUVA{
    {GAIN::_2048x, 9.75}, {GAIN::_1024x, 19.50}, {GAIN::_512x, 39.00}, {GAIN::_256x, 78.00},
    {GAIN::_128x, 156.0}, {GAIN::_64x, 312.00},  {GAIN::_32x, 624.00}, {GAIN::_16x, 1248.00},
    {GAIN::_8x, 2496.00}, {GAIN::_4x, 4992.00},  {GAIN::_2x, 9984.00}, {GAIN::_1x, 19968.00},
};

const std::unordered_map<AS7331::GAIN, double> AS7331::fullScaleRangeMapUVB{
    {GAIN::_2048x, 12.75}, {GAIN::_1024x, 25.50}, {GAIN::_512x, 51.00},  {GAIN::_256x, 102.00},
    {GAIN::_128x, 204.00}, {GAIN::_64x, 408.00},  {GAIN::_32x, 816.00},  {GAIN::_16x, 1632.00},
    {GAIN::_8x, 3264.00},  {GAIN::_4x, 6528.00},  {GAIN::_2x, 13056.00}, {GAIN::_1x, 26112.00},
};

const std::unordered_map<AS7331::GAIN, double> AS7331::fullScaleRangeMapUVC{
    {GAIN::_2048x, 6.13}, {GAIN::_1024x, 12.25}, {GAIN::_512x, 24.50}, {GAIN::_256x, 49.00},
    {GAIN::_128x, 98.00}, {GAIN::_64x, 196.00},  {GAIN::_32x, 392.00}, {GAIN::_16x, 784.00},
    {GAIN::_8x, 1568.00}, {GAIN::_4x, 3136.00},  {GAIN::_2x, 6272.00}, {GAIN::_1x, 12544.00},
};

/*
 * AS7331 UV Sensor
 */
AS7331::AS7331() : i2cDevice(0x74) {
    fTitle = fName = "AS7331";
}

AS7331::AS7331(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) {
    fTitle = fName = "AS7331";
}

AS7331::AS7331(uint8_t slaveAddress) : i2cDevice(slaveAddress) {
    fTitle = fName = "AS7331";
}

AS7331::~AS7331() {
}

bool AS7331::identify() {
    if (fMode == MODE_FAILED) {
        return false;
    }
    if (!devicePresent()) {
        return false;
    }
    uint16_t dataword{0};
    uint8_t conf_reg{0};

    return true;
}

bool AS7331::devicePresent() {
    return true;
}

void AS7331::reset() {
    std::uint8_t cmd = 0x08; // sw reset
    writeReg(registerMap.at(REG::OSR_STATUS), &cmd, 1);
}

auto AS7331::opStatus() -> Status {
    std::array<std::uint8_t, 2> buf{};
    if (readReg(registerMap.at(REG::OSR_STATUS), buf.data(), 2) < 0) {
        std::cerr << "Failed to read status from AS7331" << std::endl;
        return Status{};
    }
    std::uint8_t osr = buf.at(0);
    std::uint8_t status = buf.at(1);
    auto opStateValue = (osr & 0b111);
    OP_STATE state;
    if (opStateValue == 0b010) {
        state = OP_STATE::CONFIGURATION;
    } else if (opStateValue == 0b011) {
        state = OP_STATE::MEASUREMENT;
    } else {
        std::stringstream sstr{};
        sstr << "Invalid AS7331 operating state: " << std::hex << opStateValue << "\n";
        sstr << "Full data: " << std::hex << std::setw(2) << std::setfill(' ')
             << static_cast<unsigned>(buf.at(0));
        sstr << " " << std::hex << std::setw(2) << std::setfill(' ')
             << static_cast<unsigned>(buf.at(1)) << std::endl;
        std::cerr << sstr.str();
        // throw std::runtime_error(sstr.str());
        state = OP_STATE::UNKNOWN;
    }
    return Status{
        .startOfMeasurement = ((osr >> 7) & 0x01) == 0x01,
        .powerDownState = ((osr >> 6) & 0x01) == 0x01,
        .opState = state,
        .outConvOf = ((status >> 7) & 0x01) == 0x01,
        .mresOf = ((status >> 6) & 0x01) == 0x01,
        .adcOf = ((status >> 5) & 0x01) == 0x01,
        .lData = ((status >> 4) & 0x01) == 0x01,
        .nData = ((status >> 3) & 0x01) == 0x01,
        .notReady = ((status >> 2) & 0x01) == 0x01,
        .standby = ((status >> 1) & 0x01) == 0x01,
        .powerDown = ((status >> 0) & 0x01) == 0x01,
    };
}

void AS7331::setGain(GAIN gain) {
    std::uint8_t buf{};
    readReg(registerMap.at(REG::CREG_1), &buf, 1);
    buf &= 0xf0;
    buf |= (gainValueMap.at(gain) << 4);
    writeReg(registerMap.at(REG::CREG_1), &buf, 1);
    currentGain = gain;
}

auto AS7331::toString(const Status& s) -> std::string {
    std::stringstream ss;

    ss << "AS7331 Status {\n";

    ss << "  StartOfMeasurement: " << (s.startOfMeasurement ? "true" : "false") << "\n";
    ss << "  PowerDownState    : " << (s.powerDownState ? "true" : "false") << "\n";

    ss << "  OpState           : "
       << (s.opState == OP_STATE::CONFIGURATION
               ? "CONFIGURATION"
               : (s.opState == OP_STATE::MEASUREMENT ? "MEASUREMENT" : "UNKNOWN"))
       << "\n";

    ss << "  --- Status Flags ---\n";

    ss << "  OutConvOF         : " << (s.outConvOf ? "true" : "false") << "\n";
    ss << "  MRESOF            : " << (s.mresOf ? "true" : "false") << "\n";
    ss << "  ADC_OF            : " << (s.adcOf ? "true" : "false") << "\n";
    ss << "  LDATA             : " << (s.lData ? "true" : "false") << "\n";
    ss << "  NDATA             : " << (s.nData ? "true" : "false") << "\n";
    ss << "  NotReady          : " << (s.notReady ? "true" : "false") << "\n";
    ss << "  Standby           : " << (s.standby ? "true" : "false") << "\n";
    ss << "  PowerDown         : " << (s.powerDown ? "true" : "false") << "\n";

    ss << "}";

    return ss.str();
}

void AS7331::setOpState(OP_STATE state) {
    std::uint8_t buf{0x00};
    if (state == OP_STATE::CONFIGURATION) {
        // stop measurement
        buf = 0b010;
    }
    if (state == OP_STATE::MEASUREMENT) {
        // start measurement
        buf = 0b011;
    }
    writeReg(registerMap.at(REG::OSR_STATUS), &buf, 1);
    currentOpState = state;
}

void AS7331::startMeasurement() {
    std::uint8_t buf{0x83};
    writeReg(registerMap.at(REG::OSR_STATUS), &buf, 1);
}

void AS7331::setBreakTime(std::uint8_t time) // Step size 8µs
{
    if (currentOpState == OP_STATE::MEASUREMENT) {
        std::cerr << "Cannot set break time if in measurement mode";
        return;
    }
    writeReg(registerMap.at(REG::BREAK), &time, 1);
    currentBreakTime = time;
}

// void AS7331::setStandby(bool standby) {
//     std::uint8_t buf{};
//     readReg(registerMap.at(REG::CREG_3), &buf, 1);
//     buf = (buf & 0b11101111);
//     if (standby) {
//         buf |= (0x1 << 4);
//     }
//     writeReg(registerMap.at(REG::CREG_3), &buf, 1);
// }

auto AS7331::getConfig() -> std::string {
    std::array<std::uint8_t, 3> buf1{};
    std::stringstream sstr{};
    readReg(registerMap.at(REG::CREG_1), buf1.data(), 3);
    for (auto val : buf1) {
        sstr << std::hex << std::setw(2) << std::setfill(' ') << static_cast<unsigned>(val) << " ";
    }
    sstr << "\n";
    return sstr.str();
}

auto AS7331::readUVA() -> std::optional<double> {
    auto value = readMRES(REG::MRES_A);
    if (value.has_value() == false) {
        return std::nullopt;
    }

    return value.value() * fullScaleRangeMapUVA.at(currentGain) /
           65535.0; // 65535.0 == 0xffff == 2^16 - 1
}

auto AS7331::readUVB() -> std::optional<double> {
    auto value = readMRES(REG::MRES_B);
    if (value.has_value() == false) {
        return std::nullopt;
    }

    return value.value() * fullScaleRangeMapUVB.at(currentGain) /
           65535.0; // 65535.0 == 0xffff == 2^16 - 1
}

auto AS7331::readUVC() -> std::optional<double> {
    auto value = readMRES(REG::MRES_C);
    if (value.has_value() == false) {
        return std::nullopt;
    }

    return value.value() * fullScaleRangeMapUVC.at(currentGain) /
           65535.0; // 65535.0 == 0xffff == 2^16 - 1
}

auto AS7331::readMRES(REG reg) -> std::optional<std::uint16_t> {
    std::array<std::uint8_t, 2> buf{};
    auto result = readReg(registerMap.at(reg), buf.data(), 2);

    if (result < 0) {
        return std::nullopt;
    }

    std::uint16_t value{static_cast<std::uint16_t>(buf.at(1))};
    value = value << 8;
    value |= static_cast<std::uint16_t>(buf.at(0));
    return value;
}