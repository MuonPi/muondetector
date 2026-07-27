#ifndef OOK_TRANSMITTER_H
#define OOK_TRANSMITTER_H

#include <chrono>
#include <cstdint>
#include <gpiod.h>
#include <string>
#include <vector>

namespace MuonPi::Ook {

class Transmitter {
  public:
    struct Config {
        std::string chipPath{"/dev/gpiochip0"};
        unsigned int gpio{7};
        std::chrono::microseconds halfBitPeriod{500};
        std::chrono::milliseconds repeatGap{20};
        std::uint8_t repeats{3};
        bool activeHigh{true};
    };

    Transmitter();
    explicit Transmitter(Config config);
    ~Transmitter();

    Transmitter(const Transmitter&) = delete;
    auto operator=(const Transmitter&) -> Transmitter& = delete;

    Transmitter(Transmitter&&) = delete;
    auto operator=(Transmitter&&) -> Transmitter& = delete;

    auto open() -> bool;
    void close();

    auto sendBytes(const std::vector<std::uint8_t>& payload) -> bool;
    auto sendString(const std::string& payload) -> bool;

  private:
    auto sendFrame(const std::vector<std::uint8_t>& frame) -> bool;
    void sendManchesterByte(std::uint8_t byte);
    void sendManchesterBit(bool bit);
    void setCarrier(bool on);
    void sleepHalfBit();
    static auto buildFrame(const std::vector<std::uint8_t>& payload) -> std::vector<std::uint8_t>;
    static auto crc8(const std::vector<std::uint8_t>& data) -> std::uint8_t;

    Config config_;
    gpiod_chip* chip_{nullptr};
    gpiod_line_request* request_{nullptr};
    std::chrono::steady_clock::time_point nextEdge_{};
};

} // namespace MuonPi::Ook

#endif // OOK_TRANSMITTER_H
