#include "hardware/i2c/qmc5883.h"

#include <cerrno>
#include <chrono>
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
              << " [interval_seconds] [i2c_address] [range] [samples_per_value]\n"
              << "  interval_seconds defaults to 1.0\n"
              << "  i2c_address defaults to 0x0d\n"
              << "  range defaults to 2, valid values are 2 or 8 Gauss\n"
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

bool parseRange(const char* text, QMC5883::RANGE& range) {
    char* end = nullptr;
    errno = 0;
    const auto value = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0') {
        return false;
    }
    if (value == 2) {
        range = QMC5883::RANGE::G_2;
        return true;
    }
    if (value == 8) {
        range = QMC5883::RANGE::G_8;
        return true;
    }
    return false;
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

bool readAveragedMeasurement(QMC5883& sensor, double intervalSeconds, unsigned int samplesPerValue,
                             AveragedReading& averaged) {
    double rawX{0.0};
    double rawY{0.0};
    double rawZ{0.0};
    double xGauss{0.0};
    double yGauss{0.0};
    double zGauss{0.0};
    unsigned int validSamples{0};

    const auto windowStart = std::chrono::steady_clock::now();
    const std::chrono::duration<double> samplePeriod{intervalSeconds / samplesPerValue};

    for (unsigned int i = 0; i < samplesPerValue; ++i) {
        QMC5883::RawReading raw{};
        QMC5883::Vector3 magneticField{};
        if (sensor.readRaw(raw)) {
            const double scale = sensor.fullScaleRangeGauss() / 32768.0;
            rawX += raw.x;
            rawY += raw.y;
            rawZ += raw.z;
            magneticField.x = static_cast<double>(raw.x) * scale;
            magneticField.y = static_cast<double>(raw.y) * scale;
            magneticField.z = static_cast<double>(raw.z) * scale;
            xGauss += magneticField.x;
            yGauss += magneticField.y;
            zGauss += magneticField.z;
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
    averaged.xGauss = xGauss / validSamples;
    averaged.yGauss = yGauss / validSamples;
    averaged.zGauss = zGauss / validSamples;
    averaged.magnitudeGauss =
        std::sqrt(averaged.xGauss * averaged.xGauss + averaged.yGauss * averaged.yGauss +
                  averaged.zGauss * averaged.zGauss);
    averaged.samples = validSamples;
    return true;
}
} // namespace

int main(int argc, char* argv[]) {
    double intervalSeconds = 1.0;
    std::uint8_t address = QMC5883::DEFAULT_ADDRESS;
    QMC5883::RANGE range = QMC5883::RANGE::G_2;
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
    if (argc >= 4 && !parseRange(argv[3], range)) {
        std::cerr << "Invalid range: " << argv[3] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (argc == 5 && !parseSampleCount(argv[4], samplesPerValue)) {
        std::cerr << "Invalid sample count: " << argv[4] << "\n";
        printUsage(argv[0]);
        return 1;
    }

    QMC5883 sensor{"/dev/i2c-1", address};
    if (!sensor.init(QMC5883::MODE::CONTINUOUS, QMC5883::OUTPUT_DATA_RATE::HZ_10, range,
                     QMC5883::OVER_SAMPLE_RATIO::OSR_512)) {
        std::cerr << "QMC5883L not found at address 0x" << std::hex << static_cast<int>(address)
                  << std::dec << "\n";
        return 1;
    }

    const auto start = std::chrono::steady_clock::now();
    std::cout << "elapsed_s,raw_x,raw_y,raw_z,x_gauss,y_gauss,z_gauss,magnitude_gauss,range_gauss,"
                 "samples\n";

    while (true) {
        AveragedReading reading{};
        if (!readAveragedMeasurement(sensor, intervalSeconds, samplesPerValue, reading)) {
            std::cerr << "Read failed\n";
            continue;
        }

        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
        std::cout << std::fixed << std::setprecision(3) << elapsed.count() << ","
                  << std::setprecision(1) << reading.rawX << "," << reading.rawY << ","
                  << reading.rawZ << "," << std::setprecision(6) << reading.xGauss << ","
                  << reading.yGauss << "," << reading.zGauss << "," << reading.magnitudeGauss << ","
                  << sensor.fullScaleRangeGauss() << "," << reading.samples << "\n";
        std::cout << std::flush;
    }
}
