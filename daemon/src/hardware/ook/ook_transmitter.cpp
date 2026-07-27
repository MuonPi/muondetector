#include "ook/ook_transmitter.h"

#include <algorithm>
#include <iostream>
#include <thread>

namespace MuonPi::Ook {

namespace {
constexpr std::uint8_t PreambleByte = 0x55;
constexpr std::uint8_t SyncByte = 0x2d;
constexpr std::size_t MaxPayloadSize = 255;
} // namespace

Transmitter::Transmitter() : Transmitter(Config{}) {
}

Transmitter::Transmitter(Config config) : config_{std::move(config)} {
}

Transmitter::~Transmitter() {
    close();
}

auto Transmitter::open() -> bool {
    if (request_ != nullptr) {
        return true;
    }

    chip_ = gpiod_chip_open(config_.chipPath.c_str());
    if (chip_ == nullptr) {
        std::cerr << "OOK: failed to open " << config_.chipPath << "\n";
        return false;
    }

    auto* settings = gpiod_line_settings_new();
    auto* lineConfig = gpiod_line_config_new();
    auto* requestConfig = gpiod_request_config_new();

    if (settings == nullptr || lineConfig == nullptr || requestConfig == nullptr) {
        std::cerr << "OOK: failed to allocate gpiod configuration\n";
        gpiod_line_settings_free(settings);
        gpiod_line_config_free(lineConfig);
        gpiod_request_config_free(requestConfig);
        close();
        return false;
    }

    gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
    gpiod_line_config_add_line_settings(lineConfig, &config_.gpio, 1, settings);
    gpiod_request_config_set_consumer(requestConfig, "muonpi-ook");

    request_ = gpiod_chip_request_lines(chip_, requestConfig, lineConfig);

    gpiod_line_settings_free(settings);
    gpiod_line_config_free(lineConfig);
    gpiod_request_config_free(requestConfig);

    if (request_ == nullptr) {
        std::cerr << "OOK: failed to request GPIO " << config_.gpio << "\n";
        close();
        return false;
    }

    setCarrier(false);
    return true;
}

void Transmitter::close() {
    if (request_ != nullptr) {
        setCarrier(false);
        gpiod_line_request_release(request_);
        request_ = nullptr;
    }
    if (chip_ != nullptr) {
        gpiod_chip_close(chip_);
        chip_ = nullptr;
    }
}

auto Transmitter::sendBytes(const std::vector<std::uint8_t>& payload) -> bool {
    if (payload.size() > MaxPayloadSize) {
        std::cerr << "OOK: payload too large, max is " << MaxPayloadSize << " bytes\n";
        return false;
    }
    if (!open()) {
        return false;
    }

    const auto frame = buildFrame(payload);
    const std::uint8_t repeats = std::max<std::uint8_t>(config_.repeats, 1);
    for (std::uint8_t i = 0; i < repeats; i++) {
        if (!sendFrame(frame)) {
            return false;
        }
        setCarrier(false);
        std::this_thread::sleep_for(config_.repeatGap);
    }
    return true;
}

auto Transmitter::sendString(const std::string& payload) -> bool {
    return sendBytes(std::vector<std::uint8_t>{payload.begin(), payload.end()});
}

auto Transmitter::sendFrame(const std::vector<std::uint8_t>& frame) -> bool {
    if (request_ == nullptr) {
        return false;
    }

    nextEdge_ = std::chrono::steady_clock::now();
    for (const auto byte : frame) {
        sendManchesterByte(byte);
    }
    setCarrier(false);
    return true;
}

void Transmitter::sendManchesterByte(std::uint8_t byte) {
    for (int bit = 7; bit >= 0; bit--) {
        sendManchesterBit(((byte >> bit) & 0x01) != 0);
    }
}

void Transmitter::sendManchesterBit(bool bit) {
    // IEEE 802.3 convention: 1 = high-to-low, 0 = low-to-high.
    setCarrier(bit);
    sleepHalfBit();
    setCarrier(!bit);
    sleepHalfBit();
}

void Transmitter::setCarrier(bool on) {
    if (request_ == nullptr) {
        return;
    }

    const bool lineHigh = config_.activeHigh ? on : !on;
    gpiod_line_request_set_value(request_, config_.gpio,
                                 lineHigh ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
}

void Transmitter::sleepHalfBit() {
    nextEdge_ += config_.halfBitPeriod;
    std::this_thread::sleep_until(nextEdge_);
}

auto Transmitter::buildFrame(const std::vector<std::uint8_t>& payload)
    -> std::vector<std::uint8_t> {
    std::vector<std::uint8_t> frame;
    frame.reserve(8 + payload.size());

    for (int i = 0; i < 4; i++) {
        frame.push_back(PreambleByte);
    }
    frame.push_back(SyncByte);
    frame.push_back(static_cast<std::uint8_t>(payload.size()));
    frame.insert(frame.end(), payload.begin(), payload.end());
    frame.push_back(crc8(payload));

    return frame;
}

auto Transmitter::crc8(const std::vector<std::uint8_t>& data) -> std::uint8_t {
    std::uint8_t crc = 0x00;
    for (auto byte : data) {
        crc ^= byte;
        for (int i = 0; i < 8; i++) {
            crc = (crc & 0x80) != 0 ? static_cast<std::uint8_t>((crc << 1) ^ 0x07)
                                    : static_cast<std::uint8_t>(crc << 1);
        }
    }
    return crc;
}

} // namespace MuonPi::Ook
