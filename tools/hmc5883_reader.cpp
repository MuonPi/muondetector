#include "hardware/i2c/hmc5883.h"

#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>

namespace {
struct AveragedReading {
    double rawX{0.0};
    double rawY{0.0};
    double rawZ{0.0};
    double xGauss{0.0};
    double yGauss{0.0};
    double zGauss{0.0};
    double magnitudeGauss{0.0};
    unsigned int samples{0};
};

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName
              << " [interval_seconds] [i2c_address] [gain] [samples_per_value]\n"
              << "  interval_seconds defaults to 1.0\n"
              << "  i2c_address defaults to 0x1e\n"
              << "  gain defaults to 0, valid range is 0..7\n"
              << "  samples_per_value defaults to 5\n";
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

bool parseGain(const char* text, std::uint8_t& gain) {
    char* end = nullptr;
    errno = 0;
    const auto value = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > 7) {
        return false;
    }
    gain = static_cast<std::uint8_t>(value);
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

bool readAveragedMeasurement(HMC5883& sensor, double intervalSeconds, std::uint8_t gain,
                             unsigned int samplesPerValue, AveragedReading& averaged) {
    double rawX{0.0};
    double rawY{0.0};
    double rawZ{0.0};
    unsigned int validSamples{0};

    const auto windowStart = std::chrono::steady_clock::now();
    const std::chrono::duration<double> samplePeriod{intervalSeconds / samplesPerValue};

    for (unsigned int i = 0; i < samplesPerValue; ++i) {
        int x{0};
        int y{0};
        int z{0};
        if (sensor.getXYZRawValues(x, y, z)) {
            rawX += x;
            rawY += y;
            rawZ += z;
            ++validSamples;
        }

        std::this_thread::sleep_until(windowStart + samplePeriod * (i + 1));
    }

    if (validSamples == 0) {
        averaged = {};
        return false;
    }

    averaged.rawX = rawX / validSamples;
    averaged.rawY = rawY / validSamples;
    averaged.rawZ = rawZ / validSamples;

    const auto gaussPerLsb = HMC5883::GAIN[gain] / 1000.0;
    averaged.xGauss = averaged.rawX * gaussPerLsb;
    averaged.yGauss = averaged.rawY * gaussPerLsb;
    averaged.zGauss = averaged.rawZ * gaussPerLsb;
    averaged.magnitudeGauss =
        std::sqrt(averaged.xGauss * averaged.xGauss + averaged.yGauss * averaged.yGauss +
                  averaged.zGauss * averaged.zGauss);
    averaged.samples = validSamples;
    return true;
}
} // namespace

int main(int argc, char* argv[]) {
    double intervalSeconds = 1.0;
    std::uint8_t address = 0x1e;
    std::uint8_t gain = 0;
    unsigned int samplesPerValue = 5;

    if (argc > 5) {
        printUsage(argv[0]);
        return 1;
    }
    if (argc >= 2 && !parseDouble(argv[1], intervalSeconds)) {
        std::cerr << "Invalid interval: " << argv[1] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (argc >= 3 && !parseAddress(argv[2], address)) {
        std::cerr << "Invalid I2C address: " << argv[2] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (argc >= 4 && !parseGain(argv[3], gain)) {
        std::cerr << "Invalid gain: " << argv[3] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (argc == 5 && !parseSampleCount(argv[4], samplesPerValue)) {
        std::cerr << "Invalid sample count: " << argv[4] << "\n";
        printUsage(argv[0]);
        return 1;
    }

    HMC5883 sensor{"/dev/i2c-1", address};
    if (!sensor.init()) {
        std::cerr << "HMC5883 not found at address 0x" << std::hex << static_cast<int>(address)
                  << std::dec << "\n";
        return 1;
    }

    sensor.setGain(gain);
    int dummyX{0};
    int dummyY{0};
    int dummyZ{0};
    sensor.getXYZRawValues(dummyX, dummyY, dummyZ);

    const auto start = std::chrono::steady_clock::now();
    std::cout
        << "elapsed_s,raw_x,raw_y,raw_z,x_gauss,y_gauss,z_gauss,magnitude_gauss,gain,samples\n";

    while (true) {
        AveragedReading reading{};
        if (!readAveragedMeasurement(sensor, intervalSeconds, gain, samplesPerValue, reading)) {
            std::cerr << "Read failed\n";
            continue;
        }

        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
        std::cout << std::fixed << std::setprecision(3) << elapsed.count() << ","
                  << std::setprecision(1) << reading.rawX << "," << reading.rawY << ","
                  << reading.rawZ << "," << std::setprecision(6) << reading.xGauss << ","
                  << reading.yGauss << "," << reading.zGauss << "," << reading.magnitudeGauss << ","
                  << static_cast<int>(gain) << "," << reading.samples << "\n";
        std::cout << std::flush;
    }
}
