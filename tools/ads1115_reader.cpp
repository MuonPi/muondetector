#include "hardware/i2c/ads1115.h"

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <sstream>
#include <string>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>

namespace {
constexpr std::uint8_t DEFAULT_ADDRESS{0x48};
constexpr const char* BUS_PATH{"/dev/i2c-1"};
constexpr std::uint8_t ADS_REG_CONVERSION{0x00};
constexpr std::uint8_t ADS_REG_CONFIG{0x01};
constexpr std::uint8_t ADS_REG_LO_THRESH{0x02};
constexpr std::uint8_t ADS_REG_HI_THRESH{0x03};

struct ChannelSelection {
    bool all{true};
    unsigned int channel{0};
};

struct RegisterRead {
    bool ok{false};
    std::uint16_t value{0};
    int error{0};
};

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName
              << " [interval_seconds] [i2c_address] [channel|all] [pga_index] [rate_index]"
                 " [--probe]\n"
              << "  interval_seconds defaults to 1.0\n"
              << "  i2c_address defaults to 0x48\n"
              << "  channel defaults to all; valid channels are 0..3\n"
              << "  pga_index defaults to 1 (+/-4.096 V): 0=6.144V, 1=4.096V, 2=2.048V, "
                 "3=1.024V, 4=0.512V, 5=0.256V\n"
              << "  rate_index defaults to 7 (860 SPS): 0=8, 1=16, 2=32, 3=64, 4=128, "
                 "5=250, 6=475, 7=860\n"
              << "  --probe prints address/register diagnostics and exits\n";
}

bool isProbeFlag(const char* text) {
    const std::string value{text};
    return value == "--probe" || value == "probe";
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

std::string hexByte(std::uint8_t value) {
    std::ostringstream sstr;
    sstr << "0x" << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<unsigned int>(value);
    return sstr.str();
}

std::string hexWord(std::uint16_t value) {
    std::ostringstream sstr;
    sstr << "0x" << std::hex << std::setw(4) << std::setfill('0') << value;
    return sstr.str();
}

std::string errnoMessage(int error) {
    if (error == 0) {
        return "no errno";
    }
    std::ostringstream sstr;
    sstr << "errno " << error << " (" << std::strerror(error) << ")";
    return sstr.str();
}

const char* yesNo(bool value) {
    return value ? "yes" : "no";
}

bool isPossibleAds1115Address(std::uint8_t address) {
    return address >= 0x48 && address <= 0x4b;
}

bool selectAddress(int fd, std::uint8_t address, bool& usedForce, int& error) {
    usedForce = false;
    errno = 0;
    if (ioctl(fd, I2C_SLAVE, address) == 0) {
        error = 0;
        return true;
    }

    error = errno;
    errno = 0;
    if (ioctl(fd, I2C_SLAVE_FORCE, address) == 0) {
        usedForce = true;
        error = 0;
        return true;
    }

    error = errno;
    return false;
}

bool smbusQuick(int fd, int& error) {
    i2c_smbus_ioctl_data args{};
    args.read_write = I2C_SMBUS_WRITE;
    args.command = 0;
    args.size = I2C_SMBUS_QUICK;
    args.data = nullptr;

    errno = 0;
    if (ioctl(fd, I2C_SMBUS, &args) == 0) {
        error = 0;
        return true;
    }
    error = errno;
    return false;
}

bool receiveByte(int fd, std::uint8_t& value, int& error) {
    i2c_smbus_ioctl_data args{};
    i2c_smbus_data data{};
    args.read_write = I2C_SMBUS_READ;
    args.command = 0;
    args.size = I2C_SMBUS_BYTE;
    args.data = &data;

    errno = 0;
    if (ioctl(fd, I2C_SMBUS, &args) == 0) {
        value = static_cast<std::uint8_t>(data.byte & 0xff);
        error = 0;
        return true;
    }
    error = errno;
    return false;
}

RegisterRead readWordRegister(int fd, std::uint8_t address, std::uint8_t reg) {
    std::array<std::uint8_t, 2> buffer{};
    std::uint8_t pointer = reg;
    i2c_msg messages[2]{};
    messages[0].addr = address;
    messages[0].flags = 0;
    messages[0].len = 1;
    messages[0].buf = &pointer;
    messages[1].addr = address;
    messages[1].flags = I2C_M_RD;
    messages[1].len = static_cast<__u16>(buffer.size());
    messages[1].buf = buffer.data();

    i2c_rdwr_ioctl_data ioctlData{};
    ioctlData.msgs = messages;
    ioctlData.nmsgs = 2;

    errno = 0;
    if (ioctl(fd, I2C_RDWR, &ioctlData) < 0) {
        return {.ok = false, .error = errno};
    }

    return {.ok = true,
            .value = static_cast<std::uint16_t>((static_cast<std::uint16_t>(buffer[0]) << 8) |
                                                buffer[1]),
            .error = 0};
}

bool writeWordRegister(int fd, std::uint8_t reg, std::uint16_t value, int& error) {
    const std::array<std::uint8_t, 3> buffer{reg, static_cast<std::uint8_t>((value >> 8) & 0xff),
                                             static_cast<std::uint8_t>(value & 0xff)};

    errno = 0;
    const auto written = ::write(fd, buffer.data(), buffer.size());
    if (written == static_cast<ssize_t>(buffer.size())) {
        error = 0;
        return true;
    }
    error = errno;
    return false;
}

std::uint16_t buildSingleShotConfig(ADS1115::CFG_PGA pga, unsigned int rate) {
    std::uint16_t config{0x8000}; // start single conversion
    config |= 0x4000;             // single-ended AIN0
    config |= 0x0100;             // single-shot mode
    config |= (static_cast<std::uint16_t>(pga) & 0x07) << 9;
    config |= (static_cast<std::uint16_t>(rate) & 0x07) << 5;
    config |= 0x0003; // disable comparator
    return config;
}

bool runConversionProbe(int fd, std::uint8_t address, ADS1115::CFG_PGA pga, unsigned int rate,
                        std::ostream& os) {
    const auto probeConfig = buildSingleShotConfig(pga, rate);
    int writeError{0};
    os << "  ADS one-shot write: ";
    if (!writeWordRegister(fd, ADS_REG_CONFIG, probeConfig, writeError)) {
        os << "failed (" << errnoMessage(writeError) << ")\n";
        return false;
    }
    os << "ok config=" << hexWord(probeConfig) << "\n";

    RegisterRead config{};
    bool complete{false};
    for (unsigned int i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        config = readWordRegister(fd, address, ADS_REG_CONFIG);
        if (!config.ok) {
            os << "  ADS one-shot status: config read failed (" << errnoMessage(config.error)
               << ")\n";
            return false;
        }
        if ((config.value & 0x8000) != 0) {
            complete = true;
            break;
        }
    }

    os << "  ADS one-shot status: " << (complete ? "complete" : "timeout")
       << " config=" << hexWord(config.value) << "\n";
    if (!complete) {
        return false;
    }

    const RegisterRead conversion = readWordRegister(fd, address, ADS_REG_CONVERSION);
    os << "  ADS conversion read: ";
    if (!conversion.ok) {
        os << "failed (" << errnoMessage(conversion.error) << ")\n";
        return false;
    }

    const auto signedValue = static_cast<std::int16_t>(conversion.value);
    os << "ok raw=" << signedValue << " word=" << hexWord(conversion.value) << "\n";
    return true;
}

bool probeAddress(std::uint8_t address, ADS1115::CFG_PGA pga, unsigned int rate, std::ostream& os) {
    os << "ADS1115 probe on " << BUS_PATH << " address " << hexByte(address) << "\n";
    os << "  ADS1115 address range: " << (isPossibleAds1115Address(address) ? "possible" : "no")
       << " (valid hardware addresses are 0x48..0x4b)\n";

    errno = 0;
    const int fd = open(BUS_PATH, O_RDWR);
    if (fd < 0) {
        os << "  bus open: failed (" << errnoMessage(errno) << ")\n";
        return false;
    }
    os << "  bus open: ok\n";

    unsigned long funcs{0};
    errno = 0;
    if (ioctl(fd, I2C_FUNCS, &funcs) == 0) {
        os << "  adapter funcs: 0x" << std::hex << funcs << std::dec
           << " I2C_RDWR=" << yesNo((funcs & I2C_FUNC_I2C) != 0)
           << " SMBUS_QUICK=" << yesNo((funcs & I2C_FUNC_SMBUS_QUICK) != 0)
           << " SMBUS_BYTE=" << yesNo((funcs & I2C_FUNC_SMBUS_BYTE) != 0) << "\n";
    } else {
        os << "  adapter funcs: failed (" << errnoMessage(errno) << ")\n";
    }

    bool usedForce{false};
    int selectError{0};
    if (!selectAddress(fd, address, usedForce, selectError)) {
        os << "  address select: failed (" << errnoMessage(selectError) << ")\n";
        close(fd);
        return false;
    }
    os << "  address select: ok" << (usedForce ? " via I2C_SLAVE_FORCE" : "") << "\n";

    int quickError{0};
    os << "  SMBus quick ACK: ";
    if (smbusQuick(fd, quickError)) {
        os << "yes\n";
    } else {
        os << "no (" << errnoMessage(quickError) << ")\n";
    }

    std::uint8_t receivedByte{0};
    int receiveError{0};
    os << "  SMBus receive byte: ";
    if (receiveByte(fd, receivedByte, receiveError)) {
        os << "ok byte=" << hexByte(receivedByte) << "\n";
    } else {
        os << "failed (" << errnoMessage(receiveError) << ")\n";
    }

    const RegisterRead conversion = readWordRegister(fd, address, ADS_REG_CONVERSION);
    const RegisterRead config = readWordRegister(fd, address, ADS_REG_CONFIG);
    const RegisterRead loThresh = readWordRegister(fd, address, ADS_REG_LO_THRESH);
    const RegisterRead hiThresh = readWordRegister(fd, address, ADS_REG_HI_THRESH);
    const RegisterRead configAlias = readWordRegister(fd, address, ADS_REG_CONFIG | 0x04);

    auto printRead = [&os](const char* name, const RegisterRead& read) {
        os << "  ADS register " << name << ": ";
        if (read.ok) {
            os << "ok " << hexWord(read.value) << "\n";
        } else {
            os << "failed (" << errnoMessage(read.error) << ")\n";
        }
    };

    printRead("CONVERSION(0x00)", conversion);
    printRead("CONFIG(0x01)", config);
    printRead("LO_THRESH(0x02)", loThresh);
    printRead("HI_THRESH(0x03)", hiThresh);
    printRead("CONFIG alias 0x05", configAlias);

    const bool readableAdsRegisters = conversion.ok && config.ok && loThresh.ok && hiThresh.ok;
    const bool aliasMatches = config.ok && configAlias.ok && config.value == configAlias.value;
    const bool adsLikeReadOnly = readableAdsRegisters && aliasMatches;

    os << "  ADS pointer alias check: " << (aliasMatches ? "pass" : "fail") << "\n";
    os << "  ADS read-only signature: " << (adsLikeReadOnly ? "pass" : "fail") << "\n";

    bool conversionOk{false};
    if (adsLikeReadOnly) {
        conversionOk = runConversionProbe(fd, address, pga, rate, os);
    } else {
        os << "  ADS one-shot conversion: skipped because read-only ADS checks did not pass\n";
    }

    os << "  conclusion: ";
    if (adsLikeReadOnly && conversionOk && isPossibleAds1115Address(address)) {
        os << "address behaves like an ADS1115\n";
    } else if (adsLikeReadOnly && conversionOk) {
        os << "register behavior looks ADS1115-like, but the address is outside the normal "
              "ADS1115 hardware range\n";
    } else {
        os << "something may ACK here, but this does not look like an ADS1115\n";
    }

    if (!isPossibleAds1115Address(address)) {
        os << "  hint: ADS1115 ADDR selects only 0x48, 0x49, 0x4a, or 0x4b. Seeing "
           << hexByte(address)
           << " in i2cdetect means another device is probably present at that address.\n";
    }
    os << "  hint: i2cdetect confirms an address ACK; it does not identify the chip type or "
          "prove ADS1115 register compatibility.\n";

    close(fd);
    return adsLikeReadOnly && conversionOk && isPossibleAds1115Address(address);
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
    bool probeOnly = false;
    int valueArgc = argc;

    if (valueArgc >= 2 && isProbeFlag(argv[valueArgc - 1])) {
        probeOnly = true;
        --valueArgc;
    }

    if (valueArgc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        printUsage(argv[0]);
        return 0;
    }
    if (valueArgc > 6) {
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 2 && !parseDouble(argv[1], intervalSeconds)) {
        std::cerr << "Invalid interval: " << argv[1] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 3 && !parseAddress(argv[2], address)) {
        std::cerr << "Invalid I2C address: " << argv[2] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 4 && !parseChannel(argv[3], channels)) {
        std::cerr << "Invalid channel: " << argv[3] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc >= 5 && !parsePga(argv[4], pga)) {
        std::cerr << "Invalid PGA index: " << argv[4] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (valueArgc == 6 && !parseRate(argv[5], rate)) {
        std::cerr << "Invalid rate index: " << argv[5] << "\n";
        printUsage(argv[0]);
        return 1;
    }

    if (probeOnly) {
        return probeAddress(address, pga, rate, std::cerr) ? 0 : 2;
    }

    ADS1115 ads1115{"/dev/i2c-1", address, pga};
    if (!ads1115.identify()) {
        std::cerr << "ADS1115 not found at address 0x" << std::hex << static_cast<int>(address)
                  << std::dec << "\n";
        probeAddress(address, pga, rate, std::cerr);
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
