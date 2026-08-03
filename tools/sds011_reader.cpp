#include "hardware/sds011/sds011_parser.h"

#include <array>
#include <boost/asio.hpp>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <termios.h>
#include <variant>

namespace {
constexpr const char* DEFAULT_PORT{"/dev/ttyUSB0"};
constexpr unsigned int DEFAULT_BAUD{9600};

enum class CommandType : std::uint8_t {
    ReportingMode = 2,
    QueryData = 4,
    DeviceId = 5,
    SleepAndWork = 6,
    FirmwareVersion = 7,
    WorkingPeriod = 8
};

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [serial_port] [baud] [query_interval_seconds]\n"
              << "  serial_port defaults to " << DEFAULT_PORT << "\n"
              << "  baud defaults to " << DEFAULT_BAUD << "\n"
              << "  query_interval_seconds defaults to 1.0\n"
              << "  set query_interval_seconds to 0 for active/continuous reporting mode\n";
}

bool parseBaud(const char* text, unsigned int& value) {
    char* end = nullptr;
    errno = 0;
    const auto parsed = std::strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || parsed == 0 || parsed > 4000000U) {
        return false;
    }
    value = static_cast<unsigned int>(parsed);
    return true;
}

bool parseInterval(const char* text, double& value) {
    char* end = nullptr;
    errno = 0;
    const auto parsed = std::strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0' || !std::isfinite(parsed) || parsed < 0.0) {
        return false;
    }
    value = parsed;
    return true;
}

std::array<std::uint8_t, 19> makeCommand(CommandType type,
                                         std::optional<std::uint8_t> writeValue = std::nullopt) {
    std::array<std::uint8_t, 19> frame{};
    frame[0] = 0xAA;
    frame[1] = 0xB4;
    frame[2] = static_cast<std::uint8_t>(type);
    if (writeValue) {
        frame[3] = 0x01;
        frame[4] = *writeValue;
    }
    frame[15] = 0xFF;
    frame[16] = 0xFF;

    std::uint8_t checksum{0};
    for (std::size_t i = 2; i <= 16; ++i) {
        checksum += frame.at(i);
    }
    frame[17] = checksum;
    frame[18] = 0xAB;
    return frame;
}

bool sendCommand(boost::asio::serial_port& serial, CommandType type,
                 std::optional<std::uint8_t> writeValue = std::nullopt) {
    const auto frame = makeCommand(type, writeValue);
    boost::system::error_code ec;
    boost::asio::write(serial, boost::asio::buffer(frame), ec);
    if (ec) {
        std::cerr << "Serial write failed: " << ec.message() << "\n";
        return false;
    }
    return true;
}

const char* boolText(bool value) {
    return value ? "true" : "false";
}

void printStatus(const Sds011StatusEvent& event) {
    std::cerr << "status";
    if (event.ackType) {
        std::cerr << ",ack_type=" << static_cast<unsigned>(*event.ackType);
    }
    if (event.id) {
        std::cerr << ",sensor_id=" << *event.id;
    }
    if (event.firmwareDate) {
        std::cerr << ",firmware_date=" << *event.firmwareDate;
    }
    if (event.modeByte) {
        std::cerr << ",working_period=" << static_cast<unsigned>(*event.modeByte);
    }
    if (event.queryOnlyMode) {
        std::cerr << ",query_only=" << boolText(*event.queryOnlyMode);
    }
    if (event.sleep) {
        std::cerr << ",sleep=" << boolText(*event.sleep);
    }
    std::cerr << "\n";
}
} // namespace

int main(int argc, char* argv[]) {
    std::string port = DEFAULT_PORT;
    unsigned int baud = DEFAULT_BAUD;
    double queryIntervalSeconds = 1.0;

    if (argc == 2 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h")) {
        printUsage(argv[0]);
        return 0;
    }
    if (argc > 4) {
        printUsage(argv[0]);
        return 1;
    }
    if (argc >= 2) {
        port = argv[1];
    }
    if (argc >= 3 && !parseBaud(argv[2], baud)) {
        std::cerr << "Invalid baud rate: " << argv[2] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    if (argc == 4 && !parseInterval(argv[3], queryIntervalSeconds)) {
        std::cerr << "Invalid query interval: " << argv[3] << "\n";
        printUsage(argv[0]);
        return 1;
    }
    const auto queryInterval = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
        std::chrono::duration<double>(queryIntervalSeconds));
    if (queryIntervalSeconds > 0.0 &&
        queryInterval <= std::chrono::steady_clock::duration::zero()) {
        std::cerr << "Query interval is too small: " << queryIntervalSeconds << "\n";
        printUsage(argv[0]);
        return 1;
    }

    boost::asio::io_context io;
    boost::asio::serial_port serial(io);
    boost::system::error_code ec;
    serial.open(port, ec);
    if (ec) {
        std::cerr << "Failed to open " << port << ": " << ec.message() << "\n";
        return 1;
    }

    serial.set_option(boost::asio::serial_port_base::baud_rate(baud));
    serial.set_option(boost::asio::serial_port_base::character_size(8));
    serial.set_option(
        boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    serial.set_option(
        boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    serial.set_option(boost::asio::serial_port_base::flow_control(
        boost::asio::serial_port_base::flow_control::none));
    tcflush(serial.native_handle(), TCIOFLUSH);

    std::array<std::uint8_t, 512> rxBuffer{};
    Sds011Parser parser;
    const auto start = std::chrono::steady_clock::now();

    std::function<void()> startRead;
    startRead = [&]() {
        serial.async_read_some(boost::asio::buffer(rxBuffer), [&](boost::system::error_code readEc,
                                                                  std::size_t length) {
            if (readEc) {
                if (readEc != boost::asio::error::operation_aborted) {
                    std::cerr << "Serial read failed: " << readEc.message() << "\n";
                }
                return;
            }

            parser.feed(rxBuffer.data(), length, [&](Sds011Msg&& msg) {
                if (std::holds_alternative<Sds011Event>(msg)) {
                    const auto& event = std::get<Sds011Event>(msg);
                    const std::chrono::duration<double> elapsed =
                        std::chrono::steady_clock::now() - start;
                    std::cout << std::fixed << std::setprecision(3) << elapsed.count() << ","
                              << event.id << "," << std::setprecision(1)
                              << static_cast<double>(event.pm2dot5) / 10.0 << ","
                              << static_cast<double>(event.pm10dot0) / 10.0 << "," << event.pm2dot5
                              << "," << event.pm10dot0 << "\n";
                    std::cout << std::flush;
                } else {
                    printStatus(std::get<Sds011StatusEvent>(msg));
                }
            });

            startRead();
        });
    };

    boost::asio::signal_set signals(io, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code&, int) {
        boost::system::error_code closeEc;
        serial.close(closeEc);
        io.stop();
    });

    std::cout << "elapsed_s,sensor_id,pm2_5_ugm3,pm10_ugm3,raw_pm2_5_tenths_ugm3,"
                 "raw_pm10_tenths_ugm3\n";
    std::cout << std::flush;

    startRead();

    if (!sendCommand(serial, CommandType::SleepAndWork, 0x01)) {
        return 1;
    }
    sendCommand(serial, CommandType::FirmwareVersion);
    sendCommand(serial, CommandType::DeviceId);

    if (queryIntervalSeconds == 0.0) {
        std::cerr << "SDS011 active reporting mode on " << port << " at " << baud << " baud\n";
        sendCommand(serial, CommandType::WorkingPeriod, 0x00);
        sendCommand(serial, CommandType::ReportingMode, 0x00);
    } else {
        std::cerr << "SDS011 query-only mode on " << port << " at " << baud
                  << " baud, interval=" << queryIntervalSeconds << "s\n";
        sendCommand(serial, CommandType::WorkingPeriod, 0x00);
        sendCommand(serial, CommandType::ReportingMode, 0x01);

        boost::asio::steady_timer queryTimer(io);
        std::function<void()> scheduleQuery = [&]() {
            queryTimer.expires_after(queryInterval);
            queryTimer.async_wait([&](boost::system::error_code timerEc) {
                if (timerEc) {
                    return;
                }
                if (!sendCommand(serial, CommandType::QueryData)) {
                    io.stop();
                    return;
                }
                scheduleQuery();
            });
        };
        sendCommand(serial, CommandType::QueryData);
        scheduleQuery();
        io.run();
        return 0;
    }

    io.run();
    return 0;
}
