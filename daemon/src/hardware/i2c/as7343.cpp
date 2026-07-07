#include "i2c/as7343.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

void AS7343::printSpectrum(const std::vector<SpectralValue>& values) {
    std::vector<SpectralValue> sorted = values;

    std::sort(sorted.begin(), sorted.end(), [&](const SpectralValue& a, const SpectralValue& b) {
        return channelInfo.at(a.channel).centerWavelength <
               channelInfo.at(b.channel).centerWavelength;
    });

    std::uint16_t spectralMax = 1;
    std::uint16_t auxMax = 1;
    for (const auto& v : sorted) {
        const auto& info = channelInfo.at(v.channel);
        if (info.type == ChannelType::Spectral) {
            spectralMax = std::max(spectralMax, v.value);
        } else {
            auxMax = std::max(auxMax, v.value);
        }
    }

    constexpr int barWidth = 42;
    const auto id = ids();
    const auto s = status();
    std::string saturationReason{};
    if (s.asatAnalog && s.asatDigital) {
        saturationReason = " (analog & digital)";
    } else if (s.asatAnalog) {
        saturationReason = " (analog)";
    } else if (s.asatDigital) {
        saturationReason = " (digital)";
    }

    std::cout << "\033[2J\033[H";
    std::cout << name() << " Spectrum - Auto SMUX FIFO\n";
    std::cout << "ID: 0x" << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<unsigned>(id.id) << "  REV: " << std::dec
              << static_cast<unsigned>(id.revid) << "  AUX: " << static_cast<unsigned>(id.auxid)
              << "  Gain: " << static_cast<unsigned>(gainValue(config.gain))
              << "  Tint: " << std::fixed << std::setprecision(1) << integrationTime() / 1000.0f
              << " ms\nSaturated: " << (s.asat ? "yes" : "no") << saturationReason << "\n";
    std::cout << std::setfill(' ');
    std::cout << "============================================================\n";
    std::cout << std::left << std::setw(8) << "Channel" << std::right << std::setw(12) << "Range"
              << std::setw(10) << "Raw"
              << "  Level\n";
    std::cout << "------------------------------------------------------------\n";

    for (const auto& v : sorted) {
        const auto& info = channelInfo.at(v.channel);
        if (info.type != ChannelType::Spectral) {
            continue;
        }

        const int barLen = static_cast<int>((static_cast<float>(v.value) / spectralMax) * barWidth);

        std::ostringstream range;
        range << info.minWavelength << "-" << info.maxWavelength << " nm";

        std::cout << std::left << std::setw(8) << info.name << std::right << std::setw(12)
                  << range.str() << std::setw(10) << v.value << "  ";

        for (int i = 0; i < barLen; i++)
            std::cout << "█";

        std::cout << "\n";
    }

    std::cout << "------------------------------------------------------------\n";

    for (const auto& v : sorted) {
        const auto& info = channelInfo.at(v.channel);
        if (info.type == ChannelType::Spectral) {
            continue;
        }

        const int barLen = static_cast<int>((static_cast<float>(v.value) / auxMax) * barWidth);

        std::cout << std::left << std::setw(8) << info.name << std::right << std::setw(12) << "-"
                  << std::setw(10) << v.value << "  ";
        for (int i = 0; i < barLen; i++)
            std::cout << "█";

        std::cout << "\n";
    }

    std::cout << std::flush;
}
/**
| Channel | λ1 (nm) | λ2 (nm) | λ3 (nm) | Value |
| ------- | ------- | ------- | ------- | ----- |
| F1      | 395     | 405     | 415     | 30    |
| F2      | 415     | 425     | 435     | 22    |
| FZ      | 440     | 450     | 460     | 55    |
| F3      | 465     | 475     | 485     | 30    |
| F4      | 505     | 515     | 525     | 40    |
| FY      | 545     | 555     | 565     | 100   |
| F5      | 540     | 550     | 560     | 35    |
| FXL     | 590     | 600     | 610     | 80    |
| F6      | 630     | 640     | 650     | 50    |
| F7      | 680     | 690     | 700     | 55    |
| F8      | 735     | 745     | 755     | 60    |
| NIR     | 845     | 855     | 865     | 54    |
*/
const std::unordered_map<AS7343::SpectralChannel, AS7343::SpectralChannelInfo> AS7343::channelInfo{
    {SpectralChannel::F1, {"F1", 395, 405, 415, 30, ChannelType::Spectral}},
    {SpectralChannel::F2, {"F2", 415, 425, 435, 22, ChannelType::Spectral}},
    {SpectralChannel::Z, {"Z", 440, 450, 460, 55, ChannelType::Spectral}},
    {SpectralChannel::F3, {"F3", 465, 475, 485, 30, ChannelType::Spectral}},
    {SpectralChannel::F4, {"F4", 505, 515, 525, 40, ChannelType::Spectral}},
    {SpectralChannel::Y, {"Y", 545, 555, 565, 100, ChannelType::Spectral}},
    {SpectralChannel::F5, {"F5", 540, 550, 560, 35, ChannelType::Spectral}},
    {SpectralChannel::FXL, {"FXL", 590, 600, 610, 80, ChannelType::Spectral}},
    {SpectralChannel::F6, {"F6", 630, 640, 650, 50, ChannelType::Spectral}},
    {SpectralChannel::F7, {"F7", 680, 690, 700, 55, ChannelType::Spectral}},
    {SpectralChannel::F8, {"F8", 735, 745, 755, 60, ChannelType::Spectral}},
    {SpectralChannel::NIR, {"NIR", 845, 855, 865, 54, ChannelType::Spectral}},
    {SpectralChannel::VIS, {"VIS", 380, 593, 800, 420, ChannelType::Vis}},
    {SpectralChannel::FD, {"FD", 0, 0, 0, 0, ChannelType::Flicker}},
};

const std::unordered_map<AS7343::REG, std::uint8_t> AS7343::registerMap{
    {REG::AUXID, 0x58},     {REG::REVID, 0x59},     {REG::ID, 0x5A},
    {REG::CFG12, 0x66},     {REG::ENABLE, 0x80},    {REG::ATIME, 0x81},
    {REG::WTIME, 0x83},     {REG::SP_TH_L, 0x84},   {REG::SP_TH_H, 0x86},
    {REG::STATUS1, 0x93},   {REG::STATUS2, 0x90},   {REG::STATUS3, 0x91},
    {REG::STATUS4, 0xBC},   {REG::STATUS5, 0xBB},   {REG::ASTATUS, 0x94},
    {REG::DATA, 0x95},      {REG::DATA_END, 0xB8},  {REG::CFG0, 0xBF},
    {REG::CFG1, 0xC6},      {REG::CFG3, 0xC7},      {REG::CFG6, 0xF5},
    {REG::CFG8, 0xC9},      {REG::CFG9, 0xCA},      {REG::CFG10, 0x65},
    {REG::PERS, 0xCF},      {REG::GPIO, 0x6B},      {REG::ASTEP, 0xD4},
    {REG::CFG20, 0xD6},     {REG::LED, 0xCD},       {REG::AGC_GAIN_MAX, 0xD7},
    {REG::AZ_CONFIG, 0xDE}, {REG::FD_TIME_1, 0xE0}, {REG::FD_TIME_2, 0xE2},
    {REG::FIFO_CFG0, 0xDF}, {REG::FD_CFG_0, 0xDF},  {REG::FD_STATUS, 0xE3},
    {REG::INTENAB, 0xF9},   {REG::CONTROL, 0xFA},   {REG::FIFO_MAP, 0xFC},
    {REG::FIFO_LVL, 0xFD},  {REG::FDATA, 0xFE},
};

const std::unordered_map<AS7343::AUTO_SMUX_MODE, std::vector<AS7343::SpectralChannel>>
    AS7343::autoSmuxChannelMapping{
        {AS7343::AUTO_SMUX_MODE::_6Ch,
         {SpectralChannel::Z, SpectralChannel::Y, SpectralChannel::FXL, SpectralChannel::NIR,
          SpectralChannel::VIS, SpectralChannel::FD}},
        {AS7343::AUTO_SMUX_MODE::_12Ch,
         {SpectralChannel::Z, SpectralChannel::Y, SpectralChannel::FXL, SpectralChannel::NIR,
          SpectralChannel::VIS, SpectralChannel::FD, SpectralChannel::F2, SpectralChannel::F3,
          SpectralChannel::F4, SpectralChannel::F6, SpectralChannel::VIS, SpectralChannel::FD}},
        {AS7343::AUTO_SMUX_MODE::_18Ch,
         {
             SpectralChannel::Z,
             SpectralChannel::Y,
             SpectralChannel::FXL,
             SpectralChannel::NIR,
             SpectralChannel::VIS,
             SpectralChannel::FD,
             SpectralChannel::F2,
             SpectralChannel::F3,
             SpectralChannel::F4,
             SpectralChannel::F6,
             SpectralChannel::VIS,
             SpectralChannel::FD,
             SpectralChannel::F1,
             SpectralChannel::F7,
             SpectralChannel::F8,
             SpectralChannel::F5,
             SpectralChannel::VIS,
             SpectralChannel::FD,
         }}};

const std::unordered_map<AS7343::GAIN, std::uint8_t> AS7343::gainConfigMap{
    {GAIN::_0dot5x, 0x00}, {GAIN::_1x, 0x01},   {GAIN::_2x, 0x02},   {GAIN::_4x, 0x03},
    {GAIN::_8x, 0x04},     {GAIN::_16x, 0x05},  {GAIN::_32x, 0x06},  {GAIN::_64x, 0x07},
    {GAIN::_128x, 0x08},   {GAIN::_256x, 0x09}, {GAIN::_512x, 0x0a}, {GAIN::_1024x, 0x0b},
    {GAIN::_2048x, 0x0c},
};

/*
 * AS7343 UV Sensor
 */
AS7343::AS7343() : i2cDevice(0x39) {
    fTitle = fName = "AS7343";
}

AS7343::AS7343(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) {
    fTitle = fName = "AS7343";
}

AS7343::AS7343(uint8_t slaveAddress) : i2cDevice(slaveAddress) {
    fTitle = fName = "AS7343";
}

AS7343::~AS7343() {
}

void AS7343::setRegisterLowRange(bool low) {
    /*
        In order to access registers from 0x58 to 0x66 bit REG_BANK in register CFG0 (0xBF) needs to
       be set to “1”. For register access of registers 0x80 and above bit REG_BANK needs to be set
       to “0”.
    */
    std::uint8_t cfg0{};
    std::uint8_t mask = (0x01 << 4);
    readReg(registerMap.at(REG::CFG0), &cfg0, 1);
    if (low) {
        cfg0 |= mask;
    } else {
        cfg0 &= ~mask;
    }
    writeReg(registerMap.at(REG::CFG0), &cfg0, 1);
}

auto AS7343::readIds() -> bool {
    setRegisterLowRange(true);
    if (readReg(registerMap.at(REG::AUXID), &deviceId.auxid, 1) < 0) {
        return false;
    }
    deviceId.auxid &= 0b1111;
    if (readReg(registerMap.at(REG::REVID), &deviceId.revid, 1) < 0) {
        return false;
    }
    deviceId.revid &= 0b0111;
    if (readReg(registerMap.at(REG::ID), &deviceId.id, 1) < 0) {
        return false;
    }
    setRegisterLowRange(false);
    return true;
}

void AS7343::init(const Config& conf) {
    powerOn();
    readIds();

    config = conf;
    // Configure chip
    stopMeasurement();
    setGain(conf.gain);
    setIntegrationTime(conf.atime, conf.astep);
    setWait(conf.waitEnable, conf.wtime);
    setAutoZeroFrequency(conf.autoZeroFreq);
    setFlickerDetection(conf.flickerDetectionEnable, conf.fdTime, static_cast<GAIN>(conf.fdGain),
                        conf.agcFdGainMax);
    setLed(conf.ledAct, conf.ledDrive);
    setGpioMode(conf.gpioMode);
    setInterrupts(conf.asien, conf.spIen, conf.fIen, conf.sIen);
    setFifoThreshold(conf.fifoThreshold);
    setAutoSMUX(conf.autoSmuxMode);
    setFifoMap(conf.fifoMap);
}

void AS7343::powerOn() {
    std::uint8_t buf{0x01};
    if (writeReg(registerMap.at(REG::ENABLE), &buf, 1) < 0) {
        std::cerr << "Failed to power on" << std::endl;
    }
}

void AS7343::reset() {
    std::uint8_t buf{0b1110};
    if (writeReg(registerMap.at(REG::CONTROL), &buf, 1) < 0) {
        std::cerr << "Failed to reset" << std::endl;
    }
}

bool AS7343::identify() {
    if (fMode == MODE_FAILED) {
        return false;
    }
    if (!devicePresent()) {
        return false;
    }
    return ids().id == 0x81;
}

bool AS7343::devicePresent() {
    return readIds();
}

auto AS7343::name() const -> std::string {
    return "AS7343";
}

auto AS7343::ids() -> ID {
    return deviceId;
}

auto AS7343::getConfig() const -> Config {
    return config;
}

auto AS7343::status() -> Status {
    std::array<std::uint8_t, 5> buf{};
    readReg(registerMap.at(REG::STATUS1), &buf.at(0), 1);
    writeReg(registerMap.at(REG::STATUS1), &buf.at(0), 1);

    readReg(registerMap.at(REG::STATUS2), &buf.at(1), 1);
    writeReg(registerMap.at(REG::STATUS2), &buf.at(1), 1);

    readReg(registerMap.at(REG::STATUS3), &buf.at(2), 1);
    writeReg(registerMap.at(REG::STATUS3), &buf.at(2), 1);

    readReg(registerMap.at(REG::STATUS4), &buf.at(3), 1);
    writeReg(registerMap.at(REG::STATUS4), &buf.at(3), 1);

    readReg(registerMap.at(REG::STATUS5), &buf.at(4), 1);
    writeReg(registerMap.at(REG::STATUS5), &buf.at(4), 1);

    return Status{
        // STATUS
        .asat = ((buf.at(0) >> 7) & 0x01) == 0x01,
        .aint = ((buf.at(0) >> 3) & 0x01) == 0x01,
        .fint = ((buf.at(0) >> 2) & 0x01) == 0x01,
        .sint = ((buf.at(0) >> 0) & 0x01) == 0x01,
        // STATUS2
        .avalid = ((buf.at(1) >> 6) & 0x01) == 0x01,
        .asatDigital = ((buf.at(1) >> 4) & 0x01) == 0x01,
        .asatAnalog = ((buf.at(1) >> 3) & 0x01) == 0x01,
        .fdsatAnalog = ((buf.at(1) >> 1) & 0x01) == 0x01,
        .fdsatDigital = ((buf.at(1) >> 0) & 0x01) == 0x01,
        // STATUS3
        .intSpH = ((buf.at(2) >> 5) & 0x01) == 0x01,
        .intSpL = ((buf.at(2) >> 4) & 0x01) == 0x01,
        // STATUS4
        .fifoOv = ((buf.at(3) >> 7) & 0x01) == 0x01,
        .ovTemp = ((buf.at(3) >> 5) & 0x01) == 0x01,
        .fdTrig = ((buf.at(3) >> 4) & 0x01) == 0x01,
        .spTrig = ((buf.at(3) >> 2) & 0x01) == 0x01,
        .saiActive = ((buf.at(3) >> 1) & 0x01) == 0x01,
        .intBusy = ((buf.at(3) >> 0) & 0x01) == 0x01,
        // STATUS5
        .sintFd = ((buf.at(4) >> 3) & 0x01) == 0x01,
        .sintSmux = ((buf.at(4) >> 2) & 0x01) == 0x01,
    };
}

auto AS7343::fdStatus() -> FdStatus {
    std::uint8_t buf{};
    readReg(registerMap.at(REG::FD_STATUS), &buf, 1);

    return FdStatus{
        .fdMeasurementValid = ((buf >> 5) & 0x01) == 0x01,
        .fdSaturationDetected = ((buf >> 4) & 0x01) == 0x01,
        .fd120HzFlickerValid = ((buf >> 3) & 0x01) == 0x01,
        .fd120HzFlicker = ((buf >> 1) & 0x01) == 0x01,
        .fd100HzFlicker = ((buf >> 0) & 0x01) == 0x01,
    };
}

auto AS7343::astatus() -> AStatus {
    std::uint8_t buf{};
    readReg(registerMap.at(REG::ASTATUS), &buf, 1);

    return AStatus{
        .asatStatus = ((buf >> 7) & 0x01) == 0x01,
        .aGainStatus = (buf & 0x0f) != 0,
    };
}

auto AS7343::toString(const Status& s) -> std::string {
    std::stringstream ss;

    ss << "AS7343 Status:\n";

    ss << "  Saturation:\n";
    ss << "    ASAT: " << (s.asat ? "true" : "false") << "\n";
    ss << "    AVALID: " << (s.avalid ? "true" : "false") << "\n";

    ss << "  Spectral Interrupts:\n";
    ss << "    AINT: " << (s.aint ? "true" : "false") << "\n";
    ss << "    INT_SP_H: " << (s.intSpH ? "true" : "false") << "\n";
    ss << "    INT_SP_L: " << (s.intSpL ? "true" : "false") << "\n";

    ss << "  FIFO:\n";
    ss << "    FINT: " << (s.fint ? "true" : "false") << "\n";
    ss << "    FIFO_OV: " << (s.fifoOv ? "true" : "false") << "\n";

    ss << "  System:\n";
    ss << "    SINT: " << (s.sint ? "true" : "false") << "\n";
    ss << "    INT_BUSY: " << (s.intBusy ? "true" : "false") << "\n";

    ss << "  Saturation Details:\n";
    ss << "    ASAT_DIG: " << (s.asatDigital ? "true" : "false") << "\n";
    ss << "    ASAT_ANA: " << (s.asatAnalog ? "true" : "false") << "\n";
    ss << "    FDSAT_ANA: " << (s.fdsatAnalog ? "true" : "false") << "\n";
    ss << "    FDSAT_DIG: " << (s.fdsatDigital ? "true" : "false") << "\n";

    ss << "  SMUX / Flicker:\n";
    ss << "    SINT_FD: " << (s.sintFd ? "true" : "false") << "\n";
    ss << "    SINT_SMUX: " << (s.sintSmux ? "true" : "false") << "\n";

    ss << "  Errors:\n";
    ss << "    FD_TRIG: " << (s.fdTrig ? "true" : "false") << "\n";
    ss << "    SP_TRIG: " << (s.spTrig ? "true" : "false") << "\n";
    ss << "    OV_TEMP: " << (s.ovTemp ? "true" : "false") << "\n";

    ss << "  Power:\n";
    ss << "    SAI_ACTIVE: " << (s.saiActive ? "true" : "false") << "\n";

    return ss.str();
}

void AS7343::startMeasurement() {
    std::uint8_t buf{0x01};
    if (config.flickerDetectionEnable) {
        buf |= (1 << 6);
    }
    if (config.waitEnable) {
        buf |= (1 << 3);
    }
    buf |= (1 << 1);
    writeReg(registerMap.at(REG::ENABLE), &buf, 1);
}

void AS7343::stopMeasurement() {
    std::uint8_t buf{0x01};
    writeReg(registerMap.at(REG::ENABLE), &buf, 1);
}

// void AS7343::configureSmux(const SmuxConfig& smux)
// {
//     // 1. Put device in SMUX disabled state
//     writeReg(registerMap.at(REG::ENABLE), disableMeasurementBits());

//     // 2. Write SMUX mapping registers
//     writeReg(REG::SMUX_CH0, smux.ch0);
//     writeReg(REG::SMUX_CH1, smux.ch1);
//     writeReg(REG::SMUX_CH2, smux.ch2);
//     writeReg(REG::SMUX_CH3, smux.ch3);
//     writeReg(REG::SMUX_CH4, smux.ch4);
//     writeReg(REG::SMUX_CH5, smux.ch5);

//     // 3. Enable SMUX
//     uint8_t enable = readReg(REG::ENABLE);
//     enable |= SMUXEN;   // SMUX enable bit (datasheet)
//     writeReg(REG::ENABLE, enable);
// }

void AS7343::waitForSpectrum() {
    constexpr std::uint32_t timeout_ms = 1000;

    auto start = std::chrono::steady_clock::now();

    while (true) {
        std::uint8_t status = 0;
        if (readReg(registerMap.at(REG::STATUS5), &status, 1) < 0) {
            std::cerr << "Failed to read STATUS5 register\n";
            return;
        }

        // Check SINT (spectral interrupt = full measurement done)
        if (((status >> 2) & 0x01) == 0x01) {
            return;
        }

        // timeout protection
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >
            timeout_ms) {
            std::cerr << "waitForSMUX timeout\n";
            return;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
}

void AS7343::waitForSMUX() {
    constexpr std::uint32_t timeout_ms = 1000;

    auto start = std::chrono::steady_clock::now();

    while (true) {
        std::uint8_t status = 0;
        if (readReg(registerMap.at(REG::STATUS5), &status, 1) < 0) {
            std::cerr << "Failed to read STATUS5 register\n";
            return;
        }

        // Check SINT (spectral interrupt = full measurement done)
        if (((status >> 2) & 0x01) == 0x01) {
            return;
        }

        // timeout protection
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() >
            timeout_ms) {
            std::cerr << "waitForSMUX timeout\n";
            return;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
}

auto AS7343::readSpectrum() -> std::vector<SpectralValue> {
    std::vector<SpectralValue> spectrumBuffer;

    const auto& mapping = autoSmuxChannelMapping.at(config.autoSmuxMode);
    const std::size_t expectedChannels = mapping.size();

    spectrumBuffer.reserve(expectedChannels);

    clearFifo();
    startMeasurement();

    while (true) {
        if (status().fifoOv) {
            clearFifo();
            spectrumBuffer.clear();
            std::cerr << "AS7343 FIFO overflow, resynchronizing\n";
            continue;
        }

        auto frameOpt = readFifoFrame();
        if (!frameOpt) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            continue;
        }

        const auto& frame = *frameOpt;
        const std::size_t offset = spectrumBuffer.size();
        const std::size_t available = std::min(expectedChannels - offset, frame.channelData.size());

        for (std::size_t i = 0; i < available; i++) {
            spectrumBuffer.push_back({mapping[offset + i], frame.channelData[i]});
        }

        if (spectrumBuffer.size() >= expectedChannels) {
            printSpectrum(spectrumBuffer);
            adjustExposureIfNeeded(spectrumBuffer, status());
            spectrumBuffer.clear();
        }
    }

    return spectrumBuffer;
}

auto AS7343::adjustExposureIfNeeded(const std::vector<SpectralValue>& spectrum,
                                    const Status& status) -> bool {
    if (spectrum.empty())
        return false;

    std::uint16_t maximum = 0;
    SpectralChannel maxChannel{};

    for (const auto& value : spectrum) {
        if (value.value > maximum) {
            maximum = value.value;
            maxChannel = value.channel;
        }
    }

    std::cout << "Auto exposure: max=" << maximum << " channel=" << channelInfo.at(maxChannel).name
              << "\n";

    // Too bright -> decrease exposure
    if (maximum > config.autoExposureHighTarget) {
        // decreaseExposure();

        std::cout << "Exposure decreased\n";
    }

    // Too dark -> increase exposure
    else if (maximum < config.autoExposureLowTarget) {
        // increaseExposure();

        std::cout << "Exposure increased\n";
    }
    return true;
}

enum FifoMapBits : uint8_t {
    FIFO_CH0 = (1 << 1),
    FIFO_CH1 = (1 << 2),
    FIFO_CH2 = (1 << 3),
    FIFO_CH3 = (1 << 4),
    FIFO_CH4 = (1 << 5),
    FIFO_CH5 = (1 << 6),
    FIFO_ASTATUS = (1 << 0)
};

auto AS7343::fifoLevel() -> std::size_t {
    std::uint8_t buf{};
    readReg(registerMap.at(REG::FIFO_LVL), &buf, 1);
    return static_cast<std::size_t>(buf);
}

void AS7343::clearFifo() {
    for (std::size_t guard = 0; guard < 256 && fifoLevel() > 0; guard++) {
        const std::size_t bytesToRead = std::min<std::size_t>(fifoLevel() * 2, 32);
        std::vector<std::uint8_t> dummy(bytesToRead);
        if (readReg(registerMap.at(REG::FDATA), dummy.data(), dummy.size()) !=
            static_cast<int>(dummy.size())) {
            std::cerr << "FIFO flush failed\n";
            return;
        }
    }
}

auto AS7343::readFifoFrame() -> std::optional<FifoFrame> {
    const std::size_t frameSize = fifoFrameSize();

    if (frameSize == 0) {
        return std::nullopt;
    }

    const std::size_t requiredEntries = (frameSize + 1) / 2;
    if (fifoLevel() < requiredEntries) {
        return std::nullopt;
    }

    FifoFrame frame{.fifoMap = config.fifoMap};

    std::vector<std::uint8_t> raw(frameSize);
    if (readReg(registerMap.at(REG::FDATA), raw.data(), raw.size()) !=
        static_cast<int>(raw.size())) {
        std::cerr << "FIFO frame read failed\n";
        return std::nullopt;
    }

    const std::size_t wordCount = std::popcount(static_cast<unsigned>(config.fifoMap & 0x7e));
    frame.channelData.reserve(wordCount);

    std::size_t rawOffset = 0;
    if (config.fifoMap & FIFO_ASTATUS) {
        frame.status = raw.at(rawOffset++);
    }

    for (std::size_t i = 0; i < wordCount; i++) {
        const std::uint8_t low = raw.at(rawOffset++);
        const std::uint8_t high = raw.at(rawOffset++);

        frame.channelData.push_back((static_cast<std::uint16_t>(high) << 8) |
                                    static_cast<std::uint16_t>(low));
    }

    return frame;
}

auto AS7343::integrationTime() -> float {
    return static_cast<float>((config.atime + 1) * (config.astep + 1) * 2.78 /*µs*/);
}

auto AS7343::fifoFrameSize() -> std::size_t {
    std::size_t size = 0;

    if (config.fifoMap & FIFO_CH0)
        size += 2;
    if (config.fifoMap & FIFO_CH1)
        size += 2;
    if (config.fifoMap & FIFO_CH2)
        size += 2;
    if (config.fifoMap & FIFO_CH3)
        size += 2;
    if (config.fifoMap & FIFO_CH4)
        size += 2;
    if (config.fifoMap & FIFO_CH5)
        size += 2;

    if (config.fifoMap & FIFO_ASTATUS)
        size += 1;

    return size;
}

void AS7343::setGain(GAIN gain) {
    std::uint8_t buf{0x00};
    if (readReg(registerMap.at(REG::CFG1), &buf, 1) < 0) {
        std::cerr << "Could not read CFG1 gain register." << std::endl;
        return;
    }

    buf &= ~0x1f;
    buf |= static_cast<std::uint8_t>(gain) & 0x1f;

    if (writeReg(registerMap.at(REG::CFG1), &buf, 1) < 0) {
        std::cerr << "Could not set AS7343 gain." << std::endl;
    }
}

void AS7343::setIntegrationTime(std::uint8_t atime, std::uint16_t astep) {
    if (writeReg(registerMap.at(REG::ATIME), &atime, 1) < 0) {
        std::cerr << "Could not set AS7343 ATIME." << std::endl;
    }

    std::array<std::uint8_t, 2> astepBuf{static_cast<std::uint8_t>(astep & 0xff),
                                         static_cast<std::uint8_t>((astep >> 8) & 0xff)};

    if (writeReg(registerMap.at(REG::ASTEP), astepBuf.data(), astepBuf.size()) < 0) {
        std::cerr << "Could not set AS7343 ASTEP." << std::endl;
    }
}

void AS7343::setAutoSMUX(AUTO_SMUX_MODE mode) {
    std::uint8_t buf{0x00};
    if (readReg(registerMap.at(REG::CFG20), &buf, 1) < 0) {
        std::cerr << "Could not set Auto SMUX mode." << std::endl;
    }
    buf &= ~(0x03 << 5);
    switch (mode) {
        case AUTO_SMUX_MODE::_6Ch:
            // Leave 0x00
            break;
        case AUTO_SMUX_MODE::_12Ch:
            buf |= (0x02 << 5);
            break;
        case AUTO_SMUX_MODE::_18Ch:
            buf |= (0x03 << 5);
            break;
        default:
            throw std::logic_error("Tried to set invalid AUTO_SMUX_MODE");
            break;
    }
    if (writeReg(registerMap.at(REG::CFG20), &buf, 1) < 0) {
        std::cerr << "Could not set Auto SMUX mode." << std::endl;
    }
}

void AS7343::setFifoMap(std::uint8_t fifoMap) {
    std::uint8_t buf{static_cast<std::uint8_t>(0x7F & fifoMap)};
    if (writeReg(registerMap.at(REG::FIFO_MAP), &buf, 1) < 0) {
        std::cerr << "Failed to write FIFO_MAP" << std::endl;
    }
}

void AS7343::setFifoThreshold(FIFO_THRESHOLD threshold) {
    std::uint8_t buf{};
    switch (threshold) {
        case FIFO_THRESHOLD::_1:
            buf = 0x00;
            break;
        case FIFO_THRESHOLD::_4:
            buf = (0x01 << 6);
            break;
        case FIFO_THRESHOLD::_8:
            buf = (0x02 << 6);
            break;
        case FIFO_THRESHOLD::_16:
            buf = (0x03 << 6);
            break;
        default:
            throw std::logic_error("Invalid FIFO threshold setting");
            break;
    }
    if (writeReg(registerMap.at(REG::CFG8), &buf, 1) < 0) {
        std::cerr << "Failed to write CFG8 (FIFO Threshold)" << std::endl;
    }
}

auto AS7343::adcFullScale() const -> std::uint16_t {
    return adcFullscale;
}

auto AS7343::gainValue(GAIN gain) const -> std::uint8_t {
    return gainConfigMap.at(gain);
}

void AS7343::setWait(bool enable, std::uint8_t wtime, bool longWait) {
    if (writeReg(registerMap.at(REG::WTIME), &wtime, 1) < 0) {
        std::cerr << "Failed to write WTIME" << std::endl;
        return;
    }
    std::uint8_t buf{};
    if (readReg(registerMap.at(REG::CFG0), &buf, 1) < 0) {
        std::cerr << "Failed to read CFG0" << std::endl;
        return;
    }
    std::uint8_t wLongMask = 0b100;
    if (longWait) {
        buf |= wLongMask;
    } else {
        buf &= ~wLongMask;
    }
    if (writeReg(registerMap.at(REG::CFG0), &buf, 1) < 0) {
        std::cerr << "Failed to write CFG0" << std::endl;
        return;
    }

    // Set WEN (wait enable)
    if (readReg(registerMap.at(REG::ENABLE), &buf, 1) < 0) {
        std::cerr << "Failed to read ENABLE" << std::endl;
        return;
    }
    std::uint8_t enableMask = 0b1000;
    if (enable) {
        buf |= enableMask;
    } else {
        buf &= ~enableMask;
        ;
    }
    if (writeReg(registerMap.at(REG::ENABLE), &buf, 1) < 0) {
        std::cerr << "Failed to write ENABLE" << std::endl;
        return;
    }
}

void AS7343::setLed(bool active, std::uint8_t drive) {
    std::uint8_t buf{static_cast<std::uint8_t>(active ? (0x01U << 7) : 0x00)};
    buf |= (0b111 & drive);
    if (writeReg(registerMap.at(REG::LED), &buf, 1) < 0) {
        std::cerr << "Failed to write LED status" << std::endl;
    }
}

void AS7343::setGpioMode(std::uint8_t mode) {
    if (writeReg(registerMap.at(REG::GPIO), &mode, 1) < 0) {
        std::cerr << "Failed to write GPIO mode" << std::endl;
    }
}

void AS7343::setInterrupts(bool saturation, bool spectral, bool fifo, bool system) {
    std::uint8_t buf{};
    if (readReg(registerMap.at(REG::INTENAB), &buf, 1) < 0) {
        std::cerr << "Failed to read INTENAB register" << std::endl;
        return;
    }
    if (saturation) {
        buf |= (0x01 << 7);
    }
    if (spectral) {
        buf |= (0x01 << 3);
    }
    if (fifo) {
        buf |= (0x01 << 2);
    }
    if (system) {
        buf |= 0x01;
    }
    if (writeReg(registerMap.at(REG::INTENAB), &buf, 1) < 0) {
        std::cerr << "Failed to write INTENAB register" << std::endl;
    }
}

void AS7343::setSpectralInterruptThresholds(std::uint16_t low, std::uint16_t high,
                                            std::uint8_t channel, std::uint8_t persistence) {
}

void AS7343::setFlickerDetection(bool enable, std::uint16_t fdTime, GAIN fdGain,
                                 std::uint8_t agcGainMax) {
}

void AS7343::setAutoZeroFrequency(std::uint8_t frequency) {
    if (writeReg(registerMap.at(REG::AZ_CONFIG), &frequency, 1) < 0) {
        std::cerr << "Failed to write autozero frequency" << std::endl;
    }
}

void AS7343::setAutoExposure(bool enable) {
}

void AS7343::manualAutoZero() {
    std::uint8_t buf{0b1000};
    if (writeReg(registerMap.at(REG::CONTROL), &buf, 1) < 0) {
    }
}
