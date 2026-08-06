#include "hardware/i2c/as7343.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <unistd.h>

namespace {
void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [interval_seconds] [i2c_address]\n"
              << "  interval_seconds defaults to 1.0\n"
              << "  i2c_address defaults to 0x39\n";
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

    std::cout << "Starting measurement..." << std::endl;
    AS7343::Config config{};
    config.autoSmuxMode = AS7343::AUTO_SMUX_MODE::_18Ch;
    config.fifoMap = 0x7e; // Enable CH0..CH5. VIS and FD are part of the auto-SMUX channel data.
    config.gain = AS7343::GAIN::_16x;
    config.atime = 0x09;
    config.astep = 3596; // ~100 ms per 6-channel SMUX step: (9 + 1) * (3596 + 1) * 2.78 us
    config.ledAct = false;
    config.ledDrive = 0b0100;
    as7343.reset();
    usleep(10000);
    as7343.init(config);

    while (true) {
        auto spectrum = as7343.readSpectrum();
        as7343.printSpectrum(spectrum);
        usleep(static_cast<useconds_t>(intervalSeconds * 1000000.0));
    }
}
