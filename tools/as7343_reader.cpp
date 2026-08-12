#include "hardware/i2c/as7343.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {
struct ExposurePreset {
    const char* name;
    AS7343::GAIN gain;
    std::uint8_t atime;
    std::uint16_t astep;
};

struct Measurement {
    ExposurePreset preset;
    std::vector<AS7343::SpectralValue> spectrum;
    AS7343::Status status;
    std::uint16_t maxRaw{};
    bool saturated{};
    bool nearFullScale{};
};

constexpr std::array<ExposurePreset, 3> exposurePresets{{
    {"sun", AS7343::GAIN::_0dot5x, 0x00, 999},    // 2.78 ms, low gain
    {"partial", AS7343::GAIN::_4x, 0x00, 3596},   // ~10 ms
    {"totality", AS7343::GAIN::_16x, 0x09, 3596}, // ~100 ms
}};

void printUsage(const char* programName) {
    std::cerr
        << "Usage: " << programName << " [interval_seconds] [i2c_address]\n"
        << "  interval_seconds defaults to 1.0\n"
        << "  i2c_address defaults to 0x39\n"
        << "  stdout is one auto-selected CSV spectrum per cycle; stderr is status/errors only\n";
}

bool parseDouble(const char* text, double& value) {
    char* end = nullptr;
    errno = 0;
    value = std::strtod(text, &end);
    return errno == 0 && end != text && *end == '\0' && value > 0.0;
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

auto gainLabel(AS7343::GAIN gain) -> const char* {
    switch (gain) {
        case AS7343::GAIN::_0dot5x:
            return "0.5x";
        case AS7343::GAIN::_1x:
            return "1x";
        case AS7343::GAIN::_2x:
            return "2x";
        case AS7343::GAIN::_4x:
            return "4x";
        case AS7343::GAIN::_8x:
            return "8x";
        case AS7343::GAIN::_16x:
            return "16x";
        case AS7343::GAIN::_32x:
            return "32x";
        case AS7343::GAIN::_64x:
            return "64x";
        case AS7343::GAIN::_128x:
            return "128x";
        case AS7343::GAIN::_256x:
            return "256x";
        case AS7343::GAIN::_512x:
            return "512x";
        case AS7343::GAIN::_1024x:
            return "1024x";
        case AS7343::GAIN::_2048x:
            return "2048x";
    }

    return "unknown";
}

auto integrationTimeMs(const ExposurePreset& preset) -> double {
    return static_cast<double>(preset.atime + 1) * static_cast<double>(preset.astep + 1) * 2.78 /
           1000.0;
}

auto adcFullScale(const ExposurePreset& preset) -> std::uint16_t {
    const auto fullScale =
        static_cast<std::uint32_t>(preset.atime + 1) * static_cast<std::uint32_t>(preset.astep + 1);
    return static_cast<std::uint16_t>(std::min<std::uint32_t>(fullScale, 0xffff));
}

auto timestampMs() -> std::int64_t {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

void printCsvHeader() {
    std::cout << "sample_id,time_ms,bracket,tint_ms,gain,adc_full_scale,"
                 "status_saturated,status_analog_saturated,status_digital_saturated,"
                 "near_full_scale,max_channel,max_raw,channel,min_nm,center_nm,max_nm,raw\n";
}

auto analyzeMeasurement(ExposurePreset preset, std::vector<AS7343::SpectralValue> spectrum,
                        AS7343::Status status) -> Measurement {
    Measurement measurement{
        .preset = preset,
        .spectrum = std::move(spectrum),
        .status = status,
    };

    if (measurement.spectrum.empty()) {
        return measurement;
    }

    const auto maxIt =
        std::max_element(measurement.spectrum.begin(), measurement.spectrum.end(),
                         [](const auto& a, const auto& b) { return a.value < b.value; });

    measurement.maxRaw = maxIt != measurement.spectrum.end() ? maxIt->value : 0;
    measurement.saturated = status.asat || status.asatAnalog || status.asatDigital;
    measurement.nearFullScale =
        measurement.maxRaw >= static_cast<std::uint16_t>(adcFullScale(preset) * 9 / 10);

    return measurement;
}

auto selectBestMeasurement(const std::vector<Measurement>& measurements) -> const Measurement* {
    const Measurement* fallback = nullptr;

    for (const auto& measurement : measurements) {
        if (!measurement.spectrum.empty()) {
            fallback = &measurement;
            break;
        }
    }

    for (auto it = measurements.rbegin(); it != measurements.rend(); ++it) {
        if (!it->spectrum.empty() && !it->saturated && !it->nearFullScale) {
            return &(*it);
        }
    }

    return fallback;
}

void printCsvRows(std::uint64_t sampleId, std::int64_t timeMs, const Measurement& measurement) {
    if (measurement.spectrum.empty()) {
        return;
    }

    const auto maxIt =
        std::max_element(measurement.spectrum.begin(), measurement.spectrum.end(),
                         [](const auto& a, const auto& b) { return a.value < b.value; });
    const auto maxRaw = maxIt != measurement.spectrum.end() ? maxIt->value : 0;
    const auto& maxInfo = AS7343::channelInfo.at(maxIt->channel);
    const auto fullScale = adcFullScale(measurement.preset);

    for (const auto& value : measurement.spectrum) {
        const auto& info = AS7343::channelInfo.at(value.channel);
        std::cout << sampleId << ',' << timeMs << ',' << measurement.preset.name << ','
                  << std::fixed << std::setprecision(3) << integrationTimeMs(measurement.preset)
                  << ',' << gainLabel(measurement.preset.gain) << ',' << fullScale << ','
                  << (measurement.saturated ? 1 : 0) << ','
                  << (measurement.status.asatAnalog ? 1 : 0) << ','
                  << (measurement.status.asatDigital ? 1 : 0) << ','
                  << (measurement.nearFullScale ? 1 : 0) << ',' << maxInfo.name << ',' << maxRaw
                  << ',' << info.name << ',' << info.minWavelength << ',' << info.centerWavelength
                  << ',' << info.maxWavelength << ',' << value.value << '\n';
    }
    std::cout << std::flush;
}
} // namespace

int main(int argc, char* argv[]) {
    double intervalSeconds = 1.0;
    std::uint8_t address = 0x39;

    if (argc > 3) {
        printUsage(argv[0]);
        return 1;
    }
    if (argc >= 2 && !parseDouble(argv[1], intervalSeconds)) {
        std::cerr << "Invalid interval: " << argv[1] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (argc == 3 && !parseAddress(argv[2], address)) {
        std::cerr << "Invalid I2C address: " << argv[2] << "\n";
        printUsage(argv[0]);
        return 1;
    }

    AS7343 as7343{"/dev/i2c-1", address};

    std::cerr << "Starting AS7343 auto-selected CSV measurement on address 0x" << std::hex
              << static_cast<unsigned>(address) << std::dec << "\n";
    AS7343::Config config{};
    config.autoSmuxMode = AS7343::AUTO_SMUX_MODE::_18Ch;
    config.fifoMap = 0x7e; // Enable CH0..CH5. VIS and FD are part of the auto-SMUX channel data.
    config.autoExposure = false;
    config.gain = exposurePresets.front().gain;
    config.atime = exposurePresets.front().atime;
    config.astep = exposurePresets.front().astep;
    config.ledAct = false;
    config.ledDrive = 0b0100;
    as7343.reset();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    as7343.init(config);

    printCsvHeader();
    std::uint64_t sampleId = 0;

    while (true) {
        const auto cycleStarted = std::chrono::steady_clock::now();
        std::vector<Measurement> measurements;
        measurements.reserve(exposurePresets.size());

        for (const auto& preset : exposurePresets) {
            as7343.setGain(preset.gain);
            as7343.setIntegrationTime(preset.atime, preset.astep);

            auto spectrum = as7343.readSpectrum();
            const auto status = as7343.status();
            measurements.push_back(analyzeMeasurement(preset, std::move(spectrum), status));
        }

        if (const auto* selected = selectBestMeasurement(measurements)) {
            printCsvRows(++sampleId, timestampMs(), *selected);
        }

        const auto elapsed = std::chrono::steady_clock::now() - cycleStarted;
        const auto interval = std::chrono::duration<double>(intervalSeconds);
        if (elapsed < interval) {
            std::this_thread::sleep_for(interval - elapsed);
        }
    }
}
