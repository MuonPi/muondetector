#include "hardware/i2c/mic184.h"

#include <cerrno>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {
enum class Zone {
    Current,
    Internal,
    External,
};

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName
              << " [interval_seconds] [i2c_address] [internal|external]\n"
              << "  interval_seconds defaults to 1.0\n"
              << "  i2c_address defaults to 0x4f\n";
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

bool parseZone(const char* text, Zone& zone) {
    const std::string value{text};
    if (value == "internal") {
        zone = Zone::Internal;
        return true;
    }
    if (value == "external") {
        zone = Zone::External;
        return true;
    }
    return false;
}
} // namespace

int main(int argc, char* argv[]) {
    double intervalSeconds = 1.0;
    uint8_t address = 0x4f;
    Zone zone = Zone::Current;

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
    if (argc == 4 && !parseZone(argv[3], zone)) {
        std::cerr << "Invalid zone: " << argv[3] << "\n";
        printUsage(argv[0]);
        return 1;
    }

    MIC184 mic184{"/dev/i2c-1", address};
    if (!mic184.identify()) {
        std::cerr << "MIC184 not found at address 0x" << std::hex << static_cast<int>(address)
                  << std::dec << "\n";
        return 1;
    }

    if (zone == Zone::Internal && !mic184.setExternal(false)) {
        std::cerr << "error: failed to select internal temperature zone\n";
        return 1;
    }
    if (zone == Zone::External && !mic184.setExternal(true)) {
        std::cerr << "error: failed to select external temperature zone\n";
        return 1;
    }

    std::cout << "temperature_C zone\n";
    while (true) {
        std::cout << std::fixed << std::setprecision(2) << mic184.getTemperature() << " "
                  << (mic184.isExternal() ? "external" : "internal") << "\n";
        std::cout << std::flush;
        usleep(static_cast<useconds_t>(intervalSeconds * 1000000.0));
    }
}
