#include "i2c/ozone3click.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <limits>
#include <thread>

namespace {
constexpr std::uint8_t LMP91000_READY_MASK{0x01};
constexpr double MIKROE_EXAMPLE_SCALE{1000.0};
} // namespace

Ozone3Click::Ozone3Click()
    : Ozone3Click("/dev/i2c-1", DEFAULT_LMP91000_ADDRESS, DEFAULT_ADC_ADDRESS,
                  DEFAULT_ADC_REFERENCE_VOLTAGE, DEFAULT_LMP_REFERENCE_VOLTAGE,
                  DEFAULT_OZONE_SENSITIVITY_NA_PER_PPM, DEFAULT_ZERO_OFFSET_VOLTAGE) {
}

Ozone3Click::Ozone3Click(std::uint8_t lmpAddress)
    : Ozone3Click("/dev/i2c-1", lmpAddress, DEFAULT_ADC_ADDRESS, DEFAULT_ADC_REFERENCE_VOLTAGE,
                  DEFAULT_LMP_REFERENCE_VOLTAGE, DEFAULT_OZONE_SENSITIVITY_NA_PER_PPM,
                  DEFAULT_ZERO_OFFSET_VOLTAGE) {
}

Ozone3Click::Ozone3Click(std::uint8_t lmpAddress, std::uint8_t adcAddress)
    : Ozone3Click("/dev/i2c-1", lmpAddress, adcAddress, DEFAULT_ADC_REFERENCE_VOLTAGE,
                  DEFAULT_LMP_REFERENCE_VOLTAGE, DEFAULT_OZONE_SENSITIVITY_NA_PER_PPM,
                  DEFAULT_ZERO_OFFSET_VOLTAGE) {
}

Ozone3Click::Ozone3Click(const char* busAddress, std::uint8_t lmpAddress, std::uint8_t adcAddress,
                         double adcReferenceVoltage, double lmpReferenceVoltage,
                         double ozoneSensitivityNanoamperePerPpm, double zeroOffsetVoltage)
    : i2cDevice(busAddress, lmpAddress)
    , fLmpAddress(lmpAddress)
    , fAdcAddress(adcAddress)
    , fAdcReferenceVoltage(adcReferenceVoltage)
    , fLmpReferenceVoltage(lmpReferenceVoltage)
    , fOzoneSensitivityNanoamperePerPpm(ozoneSensitivityNanoamperePerPpm)
    , fZeroOffsetVoltage(zeroOffsetVoltage) {
    fTitle = fName = "Ozone3Click";
}

Ozone3Click::~Ozone3Click() = default;

void Ozone3Click::setLmpAddress() {
    setAddress(fLmpAddress);
}

void Ozone3Click::setAdcAddress() {
    setAddress(fAdcAddress);
}

bool Ozone3Click::readLmpRegister(Register reg, std::uint8_t& value) {
    setLmpAddress();
    return readReg(static_cast<std::uint8_t>(reg), &value, 1) == 1;
}

bool Ozone3Click::writeLmpRegister(Register reg, std::uint8_t value) {
    setLmpAddress();
    return writeReg(static_cast<std::uint8_t>(reg), &value, 1) == 1;
}

bool Ozone3Click::waitReady(unsigned int timeoutMs) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    std::uint8_t status{0};

    do {
        if (!readLmpRegister(Register::STATUS, status)) {
            return false;
        }
        if ((status & LMP91000_READY_MASK) != 0) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    } while (std::chrono::steady_clock::now() < deadline);

    return false;
}

bool Ozone3Click::configure(const Config& config) {
    if (!waitReady()) {
        return false;
    }

    if (!writeLmpRegister(Register::MODECN, static_cast<std::uint8_t>(Mode::DEEP_SLEEP))) {
        return false;
    }
    if (!writeLmpRegister(Register::LOCK, 0x00)) {
        return false;
    }
    if (!writeLmpRegister(Register::TIACN, config.tiaControl)) {
        return false;
    }
    if (!writeLmpRegister(Register::REFCN, config.referenceControl)) {
        return false;
    }

    std::uint8_t readback{0};
    if (!readLmpRegister(Register::TIACN, readback) || readback != config.tiaControl) {
        return false;
    }
    if (!readLmpRegister(Register::REFCN, readback) || readback != config.referenceControl) {
        return false;
    }

    if (!writeLmpRegister(Register::LOCK, 0x01)) {
        return false;
    }
    if (!writeLmpRegister(Register::MODECN, static_cast<std::uint8_t>(config.mode))) {
        return false;
    }
    if (!readLmpRegister(Register::MODECN, readback) ||
        (readback & 0x87) != static_cast<std::uint8_t>(config.mode)) {
        return false;
    }

    fConfig = config;
    setLmpAddress();
    return true;
}

bool Ozone3Click::init(const Config& config) {
    return configure(config);
}

bool Ozone3Click::devicePresent() {
    std::uint8_t status{0};
    const bool lmpPresent = readLmpRegister(Register::STATUS, status);

    std::uint16_t raw{0};
    const bool adcPresent = readAdcRaw(raw);

    setLmpAddress();
    return lmpPresent && adcPresent;
}

bool Ozone3Click::identify() {
    if (!waitReady()) {
        return false;
    }

    std::uint8_t lock{0};
    if (!readLmpRegister(Register::LOCK, lock)) {
        return false;
    }

    std::uint16_t raw{0};
    return readAdcRaw(raw) && (raw <= ADC_FULL_SCALE);
}

bool Ozone3Click::readAdcRaw(std::uint16_t& raw, std::uint8_t* upperByte, std::uint8_t* lowerByte) {
    setAdcAddress();

    std::array<std::uint8_t, 2> buffer{};
    const bool success =
        read(buffer.data(), static_cast<int>(buffer.size())) == static_cast<int>(buffer.size());
    setLmpAddress();

    if (!success) {
        return false;
    }

    if (upperByte) {
        *upperByte = buffer[0];
    }
    if (lowerByte) {
        *lowerByte = buffer[1];
    }

    raw = static_cast<std::uint16_t>((static_cast<std::uint16_t>(buffer[0]) << 8) | buffer[1]);
    raw &= ADC_FULL_SCALE;
    return true;
}

bool Ozone3Click::readMeasurement(Reading& reading) {
    std::uint16_t raw{0};
    if (!readAdcRaw(raw)) {
        return false;
    }

    reading.timestamp = std::chrono::steady_clock::now();
    reading.rawAdc = raw;
    reading.adcVoltage = rawToVoltage(raw);
    reading.lmpZeroVoltage = lmpZeroVoltage();
    reading.sensorCurrentNanoampere = rawToSensorCurrentNanoampere(raw);
    reading.ozonePpbv = rawToOzonePpbv(raw);
    reading.mikroEExamplePpm = rawToMikroEExamplePpm(raw);
    return true;
}

double Ozone3Click::rawToVoltage(std::uint16_t raw) const {
    return (static_cast<double>(raw & ADC_FULL_SCALE) / ADC_CODE_COUNT) * fAdcReferenceVoltage;
}

double Ozone3Click::lmpZeroVoltage() const {
    return internalZeroFraction(fConfig.referenceControl) * fLmpReferenceVoltage +
           fZeroOffsetVoltage;
}

double Ozone3Click::rawToSensorCurrentNanoampere(std::uint16_t raw) const {
    const double rtiaOhm = tiaResistanceOhm(fConfig.tiaControl);
    if (rtiaOhm <= 0.0 || !std::isfinite(rtiaOhm)) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    return ((rawToVoltage(raw) - lmpZeroVoltage()) / rtiaOhm) * 1.0e9;
}

double Ozone3Click::rawToOzonePpbv(std::uint16_t raw) const {
    const double currentNanoampere = rawToSensorCurrentNanoampere(raw);
    if (!std::isfinite(currentNanoampere) || fOzoneSensitivityNanoamperePerPpm == 0.0) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    return (currentNanoampere / fOzoneSensitivityNanoamperePerPpm) * 1000.0;
}

double Ozone3Click::internalZeroFraction(std::uint8_t referenceControl) {
    switch ((referenceControl >> 5) & 0x03) {
        case 0x00:
            return 0.20;
        case 0x01:
            return 0.50;
        case 0x02:
            return 0.67;
        default:
            return std::numeric_limits<double>::quiet_NaN();
    }
}

double Ozone3Click::tiaResistanceOhm(std::uint8_t tiaControl) {
    switch ((tiaControl >> 2) & 0x07) {
        case 0x01:
            return 2750.0;
        case 0x02:
            return 3500.0;
        case 0x03:
            return 7000.0;
        case 0x04:
            return 14000.0;
        case 0x05:
            return 35000.0;
        case 0x06:
            return 120000.0;
        case 0x07:
            return 350000.0;
        default:
            return std::numeric_limits<double>::quiet_NaN();
    }
}

double Ozone3Click::rawToMikroEExamplePpm(std::uint16_t raw) {
    return (static_cast<double>(raw & ADC_FULL_SCALE) / ADC_FULL_SCALE) * MIKROE_EXAMPLE_SCALE;
}

double Ozone3Click::getVoltage(unsigned int) {
    Reading reading{};
    if (!readMeasurement(reading)) {
        return std::nan("");
    }
    return reading.adcVoltage;
}

Ozone3Click::Sample Ozone3Click::getSample(unsigned int channel) {
    Reading reading{};
    if (!readMeasurement(reading)) {
        return InvalidSample;
    }

    Sample sample{reading.timestamp, static_cast<int>(reading.rawAdc),
                  static_cast<float>(reading.adcVoltage),
                  static_cast<float>(fAdcReferenceVoltage / ADC_CODE_COUNT), channel};
    if (fConvReadyFn) {
        fConvReadyFn(sample);
    }
    return sample;
}

bool Ozone3Click::triggerConversion(unsigned int channel) {
    Reading reading{};
    if (!readMeasurement(reading)) {
        return false;
    }

    if (fConvReadyFn) {
        Sample sample{reading.timestamp, static_cast<int>(reading.rawAdc),
                      static_cast<float>(reading.adcVoltage),
                      static_cast<float>(fAdcReferenceVoltage / ADC_CODE_COUNT), channel};
        fConvReadyFn(sample);
    }
    return true;
}

bool Ozone3Click::getOzoneRawValue(std::int16_t& ozone) {
    std::uint16_t raw{0};
    if (!readAdcRaw(raw)) {
        return false;
    }
    ozone = static_cast<std::int16_t>(std::min<std::uint16_t>(raw, ADC_FULL_SCALE));
    return true;
}

bool Ozone3Click::getOzonePpbv(double& ozonePpbv) {
    Reading reading{};
    if (!readMeasurement(reading)) {
        return false;
    }
    ozonePpbv = reading.ozonePpbv;
    return true;
}

bool Ozone3Click::getOzonePpm(double& ozonePpm) {
    double ozonePpbv{0.0};
    if (!getOzonePpbv(ozonePpbv)) {
        return false;
    }
    ozonePpm = ozonePpbv / 1000.0;
    return true;
}

bool Ozone3Click::getOzone(double& ozone) {
    return getOzonePpm(ozone);
}
