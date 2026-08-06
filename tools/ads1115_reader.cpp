#include "hardware/i2c/ads1115.h"

#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {
constexpr std::uint8_t DEFAULT_ADDRESS{0x48};

struct ChannelSelection {
    bool all{true};
    unsigned int channel{0};
};

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName
              << " [interval_seconds] [i2c_address] [channel|all] [pga_index] [rate_index]\n"
              << "  interval_seconds defaults to 1.0\n"
              << "  i2c_address defaults to 0x48\n"
              << "  channel defaults to all; valid channels are 0..3\n"
              << "  pga_index defaults to 1 (+/-4.096 V): 0=6.144V, 1=4.096V, 2=2.048V, "
                 "3=1.024V, 4=0.512V, 5=0.256V\n"
              << "  rate_index defaults to 7 (860 SPS): 0=8, 1=16, 2=32, 3=64, 4=128, "
                 "5=250, 6=475, 7=860\n";
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

bool parseChannel(const char* text, ChannelSelection& selection) {
    const std::string value{text};
    if (value == "all") {
        selection = {};
        return true;
    }

    char* end = nullptr;
    errno = 0;
    const auto channel = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || channel > 3) {
        return false;
    }
    selection.all = false;
    selection.channel = static_cast<unsigned int>(channel);
    return true;
}

bool parsePga(const char* text, ADS1115::CFG_PGA& pga) {
    char* end = nullptr;
    errno = 0;
    const auto value = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > ADS1115::PGA256MV) {
        return false;
    }
    pga = static_cast<ADS1115::CFG_PGA>(value);
    return true;
}

bool parseRate(const char* text, unsigned int& rate) {
    char* end = nullptr;
    errno = 0;
    const auto value = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || value > ADS1115::SPS860) {
        return false;
    }
    rate = static_cast<unsigned int>(value);
    return true;
}

void printSample(ADS1115::Sample sample, ADS1115& ads1115, unsigned int requestedChannel) {
    if (sample == ADS1115::InvalidSample) {
        std::cerr << "error: measurement invalid on channel " << requestedChannel << "\n";
        return;
    }

    std::cout << sample.channel << " " << sample.value << " " << std::fixed << std::setprecision(6)
              << sample.voltage << " " << sample.lsb_voltage << " " << std::setprecision(3)
              << ads1115.getLastConvTime() << " "
              << static_cast<unsigned int>(ads1115.getPga(static_cast<int>(sample.channel)))
              << "\n";
}
} // namespace

int main(int argc, char* argv[]) {
    double intervalSeconds = 1.0;
    std::uint8_t address = DEFAULT_ADDRESS;
    ChannelSelection channels;
    ADS1115::CFG_PGA pga = ADS1115::PGA4V;
    unsigned int rate = ADS1115::SPS860;

    if (argc > 6) {
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
    if (argc >= 4 && !parseChannel(argv[3], channels)) {
        std::cerr << "Invalid channel: " << argv[3] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (argc >= 5 && !parsePga(argv[4], pga)) {
        std::cerr << "Invalid PGA index: " << argv[4] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (argc == 6 && !parseRate(argv[5], rate)) {
        std::cerr << "Invalid rate index: " << argv[5] << "\n";
        printUsage(argv[0]);
        return 1;
    }

    ADS1115 ads1115{"/dev/i2c-1", address, pga};
    if (!ads1115.identify()) {
        std::cerr << "ADS1115 not found at address 0x" << std::hex << static_cast<int>(address)
                  << std::dec << "\n";
        return 1;
    }

    ads1115.setRate(rate);
    ads1115.setAGC(false);

    std::cout << "channel raw_adc voltage_V lsb_V conv_time_ms pga_index\n";
    while (true) {
        if (channels.all) {
            for (unsigned int channel = 0; channel < 4; ++channel) {
                printSample(ads1115.getSample(channel), ads1115, channel);
            }
        } else {
            printSample(ads1115.getSample(channels.channel), ads1115, channels.channel);
        }
        std::cout << std::flush;
        usleep(static_cast<useconds_t>(intervalSeconds * 1000000.0));
    }
}
