#include "sds011.h"

#include "core/component.h"
#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "sds011_parser.h"

#include <array>
#include <functional>
#include <iomanip>
#include <sstream>

Sds011::Sds011(ComponentId id, boost::asio::io_context& io, const std::string& port,
               unsigned int baud, std::uint8_t n_sleep, EventBus& bus)
    : Component(id)
    , serial_(io)
    , tx_strand_(boost::asio::make_strand(io))
    , rx_strand_(boost::asio::make_strand(io))
    , port_(port)
    , baud_(baud)
    , bus_(bus) {
    try {
        makeConnection();
    } catch (std::runtime_error& e) {
        logWarn("Trying to open " + port + " " + std::string(e.what()) +
                ". Sds011 will not be initialized!");
        return;
    }
    bus.subscribe<Sds011Event>([](const auto& event) {
        std::stringstream sstr;
        sstr << "Data:";
        sstr << " id: " << static_cast<unsigned>(event.id);
        sstr << std::fixed << std::setprecision(1);
        sstr << " PM2.5: " << static_cast<double>(event.pm2dot5) / 10.0 << " µg/m³";
        sstr << " PM10: " << static_cast<double>(event.pm10dot0) / 10.0 << " µg/m³";
        logInfo(sstr.str());
    });
    bus.subscribe<Sds011StatusEvent>([](const auto& event) {
        std::stringstream sstr;
        if (event.firmwareDate) {
            sstr << "firmware: " << event.firmwareDate.value() << "\n";
        }
        if (event.id) {
            sstr << "id: " << std::to_string(event.id.value()) << "\n";
        }
        if (event.modeByte) {
            sstr << "mode: " << std::to_string(event.modeByte.value()) << "\n";
        }
        if (event.queryOnlyMode) {
            sstr << "queryOnlyMode: " << (event.queryOnlyMode.value() ? "true" : "false") << "\n";
        }
        if (event.sleep) {
            sstr << "sleep: " << (event.sleep.value() ? "true" : "false") << "\n";
        }
        logInfo(sstr.str());
    });

    sendCommand(CommandType::SleepAndWork, 0x01); // not sleep
    sendCommand(CommandType::WorkingPeriod, n_sleep);
    sendCommand(CommandType::ReportingMode, 0x00); // Reporting mode active reporting
}

void Sds011::makeConnection() {
    boost::system::error_code ec;

    serial_.open(port_, ec);
    if (ec) {
        throw std::runtime_error(ec.message());
    }
    serial_.set_option(boost::asio::serial_port_base::baud_rate(baud_));

    serial_.set_option(boost::asio::serial_port_base::character_size(8));

    serial_.set_option(
        boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));

    serial_.set_option(
        boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

    serial_.set_option(boost::asio::serial_port_base::flow_control(
        boost::asio::serial_port_base::flow_control::none));

    int fd = serial_.native_handle();
    tcflush(fd, TCIOFLUSH);

    startAsyncRead();
}

void Sds011::startAsyncRead() {
    serial_.async_read_some(
        boost::asio::buffer(buffer_),
        boost::asio::bind_executor(rx_strand_, [this](boost::system::error_code ec,
                                                      std::size_t length) {
            if (ec) {
                handleError("read", ec);
                // retryLater();
                return;
            }

            sds011Parser.feed(reinterpret_cast<const std::uint8_t*>(buffer_.data()), length,
                              [this](Sds011Msg&& msg) {
                                  if (std::holds_alternative<Sds011Event>(msg)) {
                                      bus_.publish(std::get<Sds011Event>(msg));
                                      logInfo("Sds011 Event: " +
                                              std::to_string(std::get<Sds011Event>(msg).pm10dot0));
                                  }
                                  if (std::holds_alternative<Sds011StatusEvent>(msg)) {
                                      bus_.publish(std::get<Sds011StatusEvent>(msg));
                                      logInfo("Sds011 Status Event");
                                  }
                              });

            startAsyncRead(); // continue reading
        }));
}

void Sds011::enqueueMessage(const std::string& msg) {
    boost::asio::post(tx_strand_, [this, msg]() {
        bool write_in_progress = !tx_queue_.empty();

        tx_queue_.push(msg);

        if (!write_in_progress) {
            do_write();
        }
    });
}

std::string Sds011::makeCommand(CommandType type, std::optional<std::uint8_t> writeValue) {

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

    std::uint8_t checksum = 0;
    for (std::size_t i = 2; i <= 16; ++i) {
        checksum += frame[i];
    }

    frame[17] = checksum;
    frame[18] = 0xAB;

    return {reinterpret_cast<const char*>(frame.data()), frame.size()};
}

void Sds011::sendCommand(CommandType type, std::optional<std::uint8_t> value) {

    enqueueMessage(makeCommand(type, value));
}

void Sds011::do_write() {
    boost::asio::async_write(
        serial_, boost::asio::buffer(tx_queue_.front()),
        boost::asio::bind_executor(tx_strand_, [this](boost::system::error_code ec, std::size_t) {
            if (ec) {
                handleError("write", ec);
                return;
            }

            tx_queue_.pop();
            if (!tx_queue_.empty()) {
                do_write();
            }
        }));
}

void Sds011::query(CommandType type) {
    std::string payload{};
    payload.reserve(19);
    std::uint8_t checksum{0};
    checksum += static_cast<std::uint8_t>(type);
    checksum += 0xff;
    checksum += 0xff;
    payload += static_cast<char>(0xAA); // 0
    payload += static_cast<char>(0xB4); // 1
    payload += static_cast<char>(type); // 2
    for (std::size_t i{0}; i < 12; i++) {
        payload += static_cast<char>(0x00); // 3 - 14
    }
    payload += static_cast<char>(0xFF);     // 15
    payload += static_cast<char>(0xFF);     // 16
    payload += static_cast<char>(checksum); // 17
    payload += static_cast<char>(0xAB);     // 18
    enqueueMessage(payload);
}
