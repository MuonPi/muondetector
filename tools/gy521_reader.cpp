#include "hardware/i2c/mpu6050.h"

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>

namespace {
void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName
              << " [interval_seconds] [i2c_address] [samples_per_value]\n"
              << "  interval_seconds defaults to 1.0\n"
              << "  i2c_address defaults to 0x68\n"
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

bool readAveragedMeasurement(MPU6050& sensor, double intervalSeconds, unsigned int samplesPerValue,
                             MPU6050::Measurement& averaged) {
    MPU6050::Measurement measurement{};
    double accelX{0.0};
    double accelY{0.0};
    double accelZ{0.0};
    double gyroX{0.0};
    double gyroY{0.0};
    double gyroZ{0.0};
    double temperature{0.0};
    unsigned int validSamples{0};

    const auto windowStart = std::chrono::steady_clock::now();
    const std::chrono::duration<double> samplePeriod{intervalSeconds / samplesPerValue};

    for (unsigned int i = 0; i < samplesPerValue; ++i) {
        if (sensor.getMeasurement(measurement)) {
            accelX += measurement.accelerationG.x;
            accelY += measurement.accelerationG.y;
            accelZ += measurement.accelerationG.z;
            gyroX += measurement.gyroscopeDps.x;
            gyroY += measurement.gyroscopeDps.y;
            gyroZ += measurement.gyroscopeDps.z;
            temperature += measurement.temperatureC;
            ++validSamples;
        }

        std::this_thread::sleep_until(windowStart + samplePeriod * (i + 1));
    }

    if (validSamples == 0) {
        averaged = {};
        return false;
    }

    averaged.accelerationG.x = accelX / validSamples;
    averaged.accelerationG.y = accelY / validSamples;
    averaged.accelerationG.z = accelZ / validSamples;
    averaged.gyroscopeDps.x = gyroX / validSamples;
    averaged.gyroscopeDps.y = gyroY / validSamples;
    averaged.gyroscopeDps.z = gyroZ / validSamples;
    averaged.temperatureC = temperature / validSamples;
    averaged.valid = true;
    return true;
}
} // namespace

int main(int argc, char* argv[]) {
    double intervalSeconds = 1.0;
    std::uint8_t address = MPU6050::DEFAULT_ADDRESS;
    unsigned int samplesPerValue = 5;

    if (argc > 4) {
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
    if (argc == 4 && !parseSampleCount(argv[3], samplesPerValue)) {
        std::cerr << "Invalid sample count: " << argv[3] << "\n";
        printUsage(argv[0]);
        return 1;
    }

    MPU6050 sensor{"/dev/i2c-1", address};
    if (!sensor.identify()) {
        std::cerr << "GY-521/MPU6050 not found at address 0x" << std::hex
                  << static_cast<int>(address) << std::dec << "\n";
        return 1;
    }

    if (!sensor.init(MPU6050::GYRO_RANGE::DPS_250, MPU6050::ACCEL_RANGE::G_2,
                     MPU6050::DLPF::ACCEL_5HZ_GYRO_5HZ, 199)) {
        std::cerr << "Failed to initialize GY-521/MPU6050\n";
        return 1;
    }

    const auto start = std::chrono::steady_clock::now();
    std::cout << "elapsed_s,accel_x_g,accel_y_g,accel_z_g,gyro_x_dps,gyro_y_dps,gyro_z_dps,temp_c,"
                 "samples\n";

    while (true) {
        MPU6050::Measurement averaged{};
        if (!readAveragedMeasurement(sensor, intervalSeconds, samplesPerValue, averaged)) {
            std::cerr << "Read failed\n";
            continue;
        }

        const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - start;
        std::cout << std::fixed << std::setprecision(3) << elapsed.count() << ","
                  << std::setprecision(6) << averaged.accelerationG.x << ","
                  << averaged.accelerationG.y << "," << averaged.accelerationG.z << ","
                  << averaged.gyroscopeDps.x << "," << averaged.gyroscopeDps.y << ","
                  << averaged.gyroscopeDps.z << "," << std::setprecision(3) << averaged.temperatureC
                  << "," << samplesPerValue << "\n";
        std::cout << std::flush;
    }
}
