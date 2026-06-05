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
        sstr << " 2.5µm : " << static_cast<unsigned>(event.pm2dot5) << " µg/m³";
        sstr << " 10.0µm : " << static_cast<unsigned>(event.pm10dot0) << " µg/m³";
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
    if (n_sleep == 0) {
        setMode(Mode::Continuous);
    } else {
        setMode(Mode::Interval, n_sleep);
    }
}

void Sds011::setMode(Mode mode, std::uint8_t n_sleep) {
    std::uint8_t modeByte{};
    bool sleep = false;
    switch (mode) {
        case Mode::Continuous:
            modeByte = 0x00;
            break;
        case Mode::Sleep:
            sleep = true;
            break;
        case Mode::Interval:
            modeByte = n_sleep;
        default:
            return;
            break;
    }
    std::string payload{};
    payload.reserve(19);

    // Set interval / mode
    std::uint8_t checksum{0};

    if (sleep == false) {
        checksum += 0x08;
        checksum += 0x01;
        checksum += modeByte;
        checksum += 0xff;
        checksum += 0xff;
        payload += static_cast<char>(0xAA);     // 0
        payload += static_cast<char>(0xB4);     // 1
        payload += static_cast<char>(0x08);     // 2
        payload += static_cast<char>(0x01);     // 3
        payload += static_cast<char>(modeByte); // 4
        for (std::size_t i{0}; i < 10; i++) {
            payload += static_cast<char>(0x00); // 5 - 14
        }
        payload += static_cast<char>(0xFF);     // 15
        payload += static_cast<char>(0xFF);     // 16
        payload += static_cast<char>(checksum); // 17
        payload += static_cast<char>(0xAB);     // 18
        enqueueMessage(payload);
        payload.clear();
    }

    // Set sleep / work
    std::uint8_t sleepByte = (sleep ? 0x00 : 0x01);
    checksum = 0;
    checksum += 0x06;
    checksum += 0x01;
    checksum += sleepByte;
    checksum += 0xff;
    checksum += 0xff;
    payload += static_cast<char>(0xAA);      // 0
    payload += static_cast<char>(0xB4);      // 1
    payload += static_cast<char>(0x06);      // 2
    payload += static_cast<char>(0x01);      // 3
    payload += static_cast<char>(sleepByte); // 4
    for (std::size_t i{0}; i < 10; i++) {
        payload += static_cast<char>(0x00); // 5 - 14
    }
    payload += static_cast<char>(0xFF);     // 15
    payload += static_cast<char>(0xFF);     // 16
    payload += static_cast<char>(checksum); // 17
    payload += static_cast<char>(0xAB);     // 18
    enqueueMessage(payload);
    return;
}

void Sds011::makeConnection() {
    boost::system::error_code ec;

    serial_.open(port_, ec);
    if (ec) {
        throw std::runtime_error(ec.message());
    }

    serial_.set_option(boost::asio::serial_port_base::baud_rate(baud_));

    int fd = serial_.native_handle();
    tcflush(fd, TCIOFLUSH);

    startAsyncRead();
}

void Sds011::startAsyncRead() {
    serial_.async_read_some(
        boost::asio::buffer(buffer_),
        boost::asio::bind_executor(
            rx_strand_, [this](boost::system::error_code ec, std::size_t length) {
                if (ec) {
                    handleError("read", ec);
                    // retryLater();
                    return;
                }

                sds011Parser.feed(reinterpret_cast<const std::uint8_t*>(buffer_.data()), length,
                                  [this](Sds011Msg&& msg) {
                                      if (std::holds_alternative<Sds011Event>(msg)) {
                                          bus_.publish(std::get<Sds011Event>(msg));
                                      }
                                      if (std::holds_alternative<Sds011StatusEvent>(msg)) {
                                          bus_.publish(std::get<Sds011StatusEvent>(msg));
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
