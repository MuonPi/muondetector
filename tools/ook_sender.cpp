#include "ook/ook_transmitter.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {

auto usage(const char* argv0) -> int {
    std::cerr << "Usage: " << argv0 << " [message] [gpio=7] [half-bit-us=500] [repeats=3]\n";
    return 2;
}

auto parseUnsigned(const char* value, unsigned long maxValue, unsigned long& out) -> bool {
    char* end = nullptr;
    const auto parsed = std::strtoul(value, &end, 10);
    if (end == value || *end != '\0' || parsed > maxValue) {
        return false;
    }
    out = parsed;
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc > 5) {
        return usage(argv[0]);
    }

    std::string message = argc > 1 ? argv[1] : "STRATO";
    unsigned long gpio = 7;
    unsigned long halfBitUs = 500;
    unsigned long repeats = 3;

    if (argc > 2 && !parseUnsigned(argv[2], 57, gpio)) {
        return usage(argv[0]);
    }
    if (argc > 3 && !parseUnsigned(argv[3], 1000000, halfBitUs)) {
        return usage(argv[0]);
    }
    if (argc > 4 && !parseUnsigned(argv[4], 255, repeats)) {
        return usage(argv[0]);
    }

    MuonPi::Ook::Transmitter transmitter{
        MuonPi::Ook::Transmitter::Config{
            .gpio = static_cast<unsigned int>(gpio),
            .halfBitPeriod = std::chrono::microseconds{halfBitUs},
            .repeats = static_cast<std::uint8_t>(repeats),
        },
    };

    if (!transmitter.sendString(message)) {
        return 1;
    }

    std::cout << "Sent OOK message on GPIO " << gpio << ": " << message << "\n";
    return 0;
}
