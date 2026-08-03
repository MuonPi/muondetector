#include "hardware/i2c/ozone3click.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

namespace {
struct AveragedReading {
    double rawAdc{0.0};
    double adcVoltage{0.0};
    double lmpZeroVoltage{0.0};
    double sensorCurrentNanoampere{0.0};
    double ozonePpbv{0.0};
    double adcVoltageSum{0.0};
    double sensorCurrentNanoampereSum{0.0};
    double ozonePpbvSum{0.0};
    double adcVoltageStddevMillivolt{0.0};
    double ozonePpbvStddev{0.0};
    std::uint16_t minRawAdc{0};
    std::uint16_t maxRawAdc{0};
    unsigned int samples{0};
};

struct CumulativeReading {
    double adcVoltage{0.0};
    double sensorCurrentNanoampere{0.0};
    double ozonePpbv{0.0};
    std::uint64_t samples{0};
};

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName
              << " [interval_seconds] [lmp91000_address] [mcp3221_address] "
                 "[adc_reference_voltage] [samples_per_value] [tia_control] "
                 "[reference_control] [mode] [lmp_reference_voltage] "
                 "[sensitivity_nA_per_ppm] [zero_offset_voltage] [--debug]\n"
              << "  interval_seconds defaults to 1.0\n"
              << "  lmp91000_address defaults to 0x48\n"
              << "  mcp3221_address defaults to 0x4d\n"
              << "  adc_reference_voltage defaults to 3.3; use 5.0 if JP1 is set to 5V\n"
              << "  samples_per_value defaults to 32\n"
              << "  tia_control defaults to 0x1f: 350 kOhm internal RTIA, 100 ohm RLOAD\n"
              << "  reference_control defaults to 0xa0: external VREF, 50% zero, 0% bias\n"
              << "  mode defaults to 0x03: 3-lead amperometric mode\n"
              << "  lmp_reference_voltage defaults to 2.048 from the MCP1501T-20E/CHY\n"
              << "  sensitivity_nA_per_ppm defaults to -60 from the 3SP-O3-20 datasheet; "
                 "use your sensor label/calibration sheet if available\n"
              << "  zero_offset_voltage defaults to 0.0; use this for zero-air calibration\n"
              << "  --debug appends cumulative all-sample average columns\n";
}

bool isDebugFlag(const char* text) {
    const std::string value{text};
    return value == "--debug" || value == "debug";
}

bool parseDouble(const char* text, double& value, double minExclusive = 0.0) {
    char* end = nullptr;
    errno = 0;
    value = std::strtod(text, &end);
    return errno == 0 && end != text && *end == '\0' && value > minExclusive;
}

bool parseFiniteDouble(const char* text, double& value) {
    char* end = nullptr;
    errno = 0;
    value = std::strtod(text, &end);
    return errno == 0 && end != text && *end == '\0' && std::isfinite(value);
}

bool parseAddress(const char* text, std::uint8_t& address) {
    char* end = nullptr;
    errno = 0;
    const auto value = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > 0x7f) {
        return false;
    }
    address = static_cast<std::uint8_t>(value);
    return true;
}

bool parseSampleCount(const char* text, unsigned int& samples) {
    char* end = nullptr;
    errno = 0;
    const auto value = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value == 0 || value > 1000) {
        return false;
    }
    samples = static_cast<unsigned int>(value);
    return true;
}

bool parseByte(const char* text, std::uint8_t& value) {
    char* end = nullptr;
    errno = 0;
    const auto parsed = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || parsed > 0xff) {
        return false;
    }
    value = static_cast<std::uint8_t>(parsed);
    return true;
}

bool readAveragedMeasurement(Ozone3Click& sensor, double intervalSeconds,
                             unsigned int samplesPerValue, AveragedReading& averaged) {
    double rawAdc{0.0};
    double adcVoltage{0.0};
    double lmpZeroVoltage{0.0};
    double sensorCurrentNanoampere{0.0};
    double ozonePpbv{0.0};
    double adcVoltageSquares{0.0};
    double ozonePpbvSquares{0.0};
    std::uint16_t minRawAdc{std::numeric_limits<std::uint16_t>::max()};
    std::uint16_t maxRawAdc{0};
    unsigned int validSamples{0};

    const auto windowStart = std::chrono::steady_clock::now();
    const std::chrono::duration<double> samplePeriod{intervalSeconds / samplesPerValue};

    for (unsigned int i = 0; i < samplesPerValue; ++i) {
        std::uint16_t raw{0};
        if (sensor.readAdcRaw(raw)) {
            const double voltage = sensor.rawToVoltage(raw);
            const double ozone = sensor.rawToOzonePpbv(raw);
            rawAdc += raw;
            adcVoltage += voltage;
            lmpZeroVoltage += sensor.lmpZeroVoltage();
            sensorCurrentNanoampere += sensor.rawToSensorCurrentNanoampere(raw);
            ozonePpbv += ozone;
            adcVoltageSquares += voltage * voltage;
            ozonePpbvSquares += ozone * ozone;
            minRawAdc = std::min(minRawAdc, raw);
            maxRawAdc = std::max(maxRawAdc, raw);
            ++validSamples;
        }

        std::this_thread::sleep_until(windowStart + samplePeriod * (i + 1));
    }

    if (validSamples == 0) {
        averaged = {};
        return false;
    }

    averaged.rawAdc = rawAdc / validSamples;
    averaged.adcVoltage = adcVoltage / validSamples;
    averaged.lmpZeroVoltage = lmpZeroVoltage / validSamples;
    averaged.sensorCurrentNanoampere = sensorCurrentNanoampere / validSamples;
    averaged.ozonePpbv = ozonePpbv / validSamples;
    averaged.adcVoltageSum = adcVoltage;
    averaged.sensorCurrentNanoampereSum = sensorCurrentNanoampere;
    averaged.ozonePpbvSum = ozonePpbv;
    const double voltageVariance =
        std::max(0.0, adcVoltageSquares / validSamples - averaged.adcVoltage * averaged.adcVoltage);
    const double ozoneVariance =
        std::max(0.0, ozonePpbvSquares / validSamples - averaged.ozonePpbv * averaged.ozonePpbv);
    averaged.adcVoltageStddevMillivolt = std::sqrt(voltageVariance) * 1000.0;
    averaged.ozonePpbvStddev = std::sqrt(ozoneVariance);
    averaged.minRawAdc = minRawAdc;
    averaged.maxRawAdc = maxRawAdc;
    averaged.samples = validSamples;
    return true;
}

CumulativeReading updateCumulativeReading(CumulativeReading cumulative,
                                          const AveragedReading& reading) {
    if (reading.samples == 0) {
        return cumulative;
    }

    const double totalSamples = static_cast<double>(cumulative.samples + reading.samples);
    cumulative.adcVoltage =
        (cumulative.adcVoltage * static_cast<double>(cumulative.samples) + reading.adcVoltageSum) /
        totalSamples;
    cumulative.sensorCurrentNanoampere =
        (cumulative.sensorCurrentNanoampere * static_cast<double>(cumulative.samples) +
         reading.sensorCurrentNanoampereSum) /
        totalSamples;
    cumulative.ozonePpbv =
        (cumulative.ozonePpbv * static_cast<double>(cumulative.samples) + reading.ozonePpbvSum) /
        totalSamples;
    cumulative.samples += reading.samples;
    return cumulative;
}

void printRegister(std::ostream& os, Ozone3Click& sensor, Ozone3Click::Register reg,
                   const char* name) {
    std::uint8_t value{0};
    if (sensor.readLmpRegister(reg, value)) {
        os << name << "=0x" << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<unsigned>(value) << std::dec << std::setfill(' ');
    } else {
        os << name << "=read_failed";
    }
}

void printDerivedCalibration(std::ostream& os, const Ozone3Click& sensor) {
    const double rtiaOhm = Ozone3Click::tiaResistanceOhm(sensor.config().tiaControl);
    const double adcVoltPerCount = sensor.adcReferenceVoltage() / Ozone3Click::ADC_CODE_COUNT;
    const double zeroAdc =
        sensor.lmpZeroVoltage() / sensor.adcReferenceVoltage() * Ozone3Click::ADC_CODE_COUNT;
    const double nanoamperePerCount = (adcVoltPerCount / rtiaOhm) * 1.0e9;
    const double ppbvPerCount =
        (nanoamperePerCount / sensor.ozoneSensitivityNanoamperePerPpm()) * 1000.0;

    os << std::fixed << std::setprecision(3) << "Calibration model: zero_adc=" << zeroAdc
       << " counts, nA_per_count=" << nanoamperePerCount << ", ppbv_per_count=" << ppbvPerCount
       << ", nominal_range=0..>20ppm, nominal_response=<15s\n"
       << std::defaultfloat;
}
} // namespace

int main(int argc, char* argv[]) {
    double intervalSeconds = 1.0;
    std::uint8_t lmpAddress = Ozone3Click::DEFAULT_LMP91000_ADDRESS;
    std::uint8_t adcAddress = Ozone3Click::DEFAULT_ADC_ADDRESS;
    double adcReferenceVoltage = Ozone3Click::DEFAULT_ADC_REFERENCE_VOLTAGE;
    double lmpReferenceVoltage = Ozone3Click::DEFAULT_LMP_REFERENCE_VOLTAGE;
    double sensorSensitivityNanoamperePerPpm = Ozone3Click::DEFAULT_OZONE_SENSITIVITY_NA_PER_PPM;
    double zeroOffsetVoltage = Ozone3Click::DEFAULT_ZERO_OFFSET_VOLTAGE;
    unsigned int samplesPerValue = 32;
    std::uint8_t tiaControl = Ozone3Click::DEFAULT_TIA_CONTROL;
    std::uint8_t referenceControl = Ozone3Click::DEFAULT_REFERENCE_CONTROL;
    std::uint8_t mode = static_cast<std::uint8_t>(Ozone3Click::Mode::THREE_LEAD);
    bool debugOutput = false;
    int valueArgc = argc;

    if (valueArgc >= 2 && isDebugFlag(argv[valueArgc - 1])) {
        debugOutput = true;
        --valueArgc;
    }

    if (valueArgc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        printUsage(argv[0]);
        return 0;
    }
    if (valueArgc > 12) {
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 2 && !parseDouble(argv[1], intervalSeconds)) {
        std::cerr << "Invalid interval: " << argv[1] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 3 && !parseAddress(argv[2], lmpAddress)) {
        std::cerr << "Invalid LMP91000 I2C address: " << argv[2] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 4 && !parseAddress(argv[3], adcAddress)) {
        std::cerr << "Invalid MCP3221 I2C address: " << argv[3] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 5 && !parseDouble(argv[4], adcReferenceVoltage)) {
        std::cerr << "Invalid ADC reference voltage: " << argv[4] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 6 && !parseSampleCount(argv[5], samplesPerValue)) {
        std::cerr << "Invalid sample count: " << argv[5] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 7 && !parseByte(argv[6], tiaControl)) {
        std::cerr << "Invalid TIA control byte: " << argv[6] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 8 && !parseByte(argv[7], referenceControl)) {
        std::cerr << "Invalid reference control byte: " << argv[7] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 9 && !parseByte(argv[8], mode)) {
        std::cerr << "Invalid mode byte: " << argv[8] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 10 && !parseDouble(argv[9], lmpReferenceVoltage)) {
        std::cerr << "Invalid LMP reference voltage: " << argv[9] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 11 && (!parseFiniteDouble(argv[10], sensorSensitivityNanoamperePerPpm) ||
                            sensorSensitivityNanoamperePerPpm == 0.0)) {
        std::cerr << "Invalid sensitivity in nA/ppm: " << argv[10] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 12 && !parseFiniteDouble(argv[11], zeroOffsetVoltage)) {
        std::cerr << "Invalid zero offset voltage: " << argv[11] << "\n";
        printUsage(argv[0]);
        return 1;
    }

    Ozone3Click sensor{"/dev/i2c-1",        lmpAddress,          adcAddress,
                       adcReferenceVoltage, lmpReferenceVoltage, sensorSensitivityNanoamperePerPpm,
                       zeroOffsetVoltage};
    if (!sensor.devicePresent()) {
        std::cerr << "Ozone 3 Click not found. Expected LMP91000 at 0x" << std::hex
                  << static_cast<unsigned>(lmpAddress) << " and MCP3221 at 0x"
                  << static_cast<unsigned>(adcAddress) << std::dec << "\n"
                  << "Check the I2C bus, RST/MENB low state, AN SEL=ADC, and address conflicts.\n";
        return 1;
    }

    Ozone3Click::Config config{tiaControl, referenceControl, static_cast<Ozone3Click::Mode>(mode)};
    if (!sensor.init(config)) {
        std::cerr << "Failed to configure Ozone 3 Click LMP91000\n";
        return 1;
    }

    std::cerr << "Ozone 3 Click configured on /dev/i2c-1: LMP91000=0x" << std::hex
              << static_cast<unsigned>(sensor.lmpAddress()) << " MCP3221=0x"
              << static_cast<unsigned>(sensor.adcAddress()) << std::dec
              << " adc_ref=" << sensor.adcReferenceVoltage()
              << "V lmp_ref=" << sensor.lmpReferenceVoltage()
              << "V sensitivity=" << sensor.ozoneSensitivityNanoamperePerPpm()
              << "nA/ppm zero_offset=" << sensor.zeroOffsetVoltage() << "V\n";
    std::cerr << "LMP91000 ";
    printRegister(std::cerr, sensor, Ozone3Click::Register::STATUS, "STATUS");
    std::cerr << " ";
    printRegister(std::cerr, sensor, Ozone3Click::Register::LOCK, "LOCK");
    std::cerr << " ";
    printRegister(std::cerr, sensor, Ozone3Click::Register::TIACN, "TIACN");
    std::cerr << " ";
    printRegister(std::cerr, sensor, Ozone3Click::Register::REFCN, "REFCN");
    std::cerr << " ";
    printRegister(std::cerr, sensor, Ozone3Click::Register::MODECN, "MODECN");
    std::cerr << "\n";
    printDerivedCalibration(std::cerr, sensor);
    std::cerr << "Note: ozone_ppbv_est uses LMP zero, RTIA, and sensitivity. It still needs "
                 "individual sensor zero/sensitivity calibration for flight-quality data.\n";

    const auto start = std::chrono::steady_clock::now();
    std::cout << "elapsed_s,raw_adc_min_u16,raw_adc_max_u16,"
                 "adc_voltage_avg,adc_stddev_mV,lmp_zero_voltage,"
                 "sensor_current_nA,ozone_ppbv_est,ozone_stddev_ppbv,samples";
    if (debugOutput) {
        std::cout << ",adc_voltage_all_avg,sensor_current_all_nA,ozone_all_ppbv,total_samples";
    }
    std::cout << "\n";
    bool fullScaleWarningPrinted = false;
    CumulativeReading cumulative{};

    while (true) {
        AveragedReading reading{};
        if (!readAveragedMeasurement(sensor, intervalSeconds, samplesPerValue, reading)) {
            std::cerr << "Read failed\n";
            continue;
        }
        if (debugOutput) {
            cumulative = updateCumulativeReading(cumulative, reading);
        }
        if (!fullScaleWarningPrinted &&
            reading.rawAdc > static_cast<double>(Ozone3Click::ADC_FULL_SCALE) * 0.98) {
            std::cerr << "Warning: MCP3221 is near full scale. Check SW1 AN SEL=ADC, "
                         "RST/MENB low, LMP register values, sensor seating, and VOUT/AIN with "
                         "a multimeter.\n";
            fullScaleWarningPrinted = true;
        }

        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
        std::cout << std::fixed << std::setprecision(3) << elapsed.count() << ","
                  << reading.minRawAdc << "," << reading.maxRawAdc << "," << std::setprecision(6)
                  << reading.adcVoltage << "," << std::setprecision(3)
                  << reading.adcVoltageStddevMillivolt << "," << std::setprecision(6)
                  << reading.lmpZeroVoltage << "," << std::setprecision(6)
                  << reading.sensorCurrentNanoampere << "," << std::setprecision(3)
                  << reading.ozonePpbv << "," << reading.ozonePpbvStddev << "," << reading.samples;
        if (debugOutput) {
            std::cout << "," << std::setprecision(6) << cumulative.adcVoltage << ","
                      << cumulative.sensorCurrentNanoampere << "," << std::setprecision(3)
                      << cumulative.ozonePpbv << "," << cumulative.samples;
        }
        std::cout << "\n";
        std::cout << std::flush;
    }
}
