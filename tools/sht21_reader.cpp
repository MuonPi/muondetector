#include "hardware/i2c/sht21.h"

#include <cerrno>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <unistd.h>

namespace {
void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [interval_seconds] [i2c_address]\n"
              << "  interval_seconds defaults to 1.0\n"
              << "  i2c_address defaults to 0x40\n";
}

bool parseDouble(const char* text, double& value) {
    char* end = nullptr;
    errno = 0;
    value = std::strtod(text, &end);
    return errno == 0 && end != text && *end == '\0' && value > 0.0;
}

bool parseAddress(const char* text, uint8_t& address) {
    char* end = nullptr;
    errno = 0;
    const auto value = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > 0x7f) {
        return false;
    }
    address = static_cast<uint8_t>(value);
    return true;
}
} // namespace

int main(int argc, char* argv[]) {
    double intervalSeconds = 1.0;
    uint8_t address = 0x40;

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

    SHT21 sht21{"/dev/i2c-1", address};
    if (!sht21.devicePresent()) {
        std::cerr << "SHT21 not found at address 0x" << std::hex << static_cast<int>(address)
                  << std::dec << "\n";
        return 1;
    }

    std::cout << "temperature_C humidity_percent\n";
    while (true) {
        const auto temperature = sht21.getTemperature();
        const auto humidity = sht21.getHumidity();
        std::cout << std::fixed << std::setprecision(2) << temperature << " " << humidity << "\n";
        std::cout << std::flush;
        usleep(static_cast<useconds_t>(intervalSeconds * 1000000.0));
    }
}
