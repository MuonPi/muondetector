#ifndef SDS011_H
#define SDS011_H

#include "core/component.h"
#include "core/event_bus.h"
#include "data/events/sds011_event.h"
#include "sds011_parser.h"

#include <boost/asio.hpp>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>

class Sds011 : public Component {
  public:
    enum class Mode { Continuous, Sleep, Interval };
    enum class CommandType {
        ReportingMode = 2,
        QueryData = 4,
        DeviceID = 5,
        SleepAndWork = 6,
        FirmwareVersion = 7,
        WorkingPeriod = 8
    };
    Sds011(ComponentId id, boost::asio::io_context& io, const std::string& port, unsigned int baud,
           std::uint8_t n_sleep, EventBus& bus);

    /**
     * n_sleep: work 30s and then sleep n*60s - 30s
     */
    void setMode(Mode mode, std::uint8_t n_sleep = 1);
    void query(CommandType type);

  private:
    void makeConnection();
    void startAsyncRead();

    void handleError(const std::string& where, const boost::system::error_code& ec) {
        std::cerr << "Error in " << where << ": " << ec.message() << std::endl;
    }
    void enqueueMessage(const std::string& msg);
    void do_write();

    boost::asio::serial_port serial_;
    std::queue<std::string> tx_queue_;
    boost::asio::strand<boost::asio::io_context::executor_type> tx_strand_;
    boost::asio::strand<boost::asio::io_context::executor_type> rx_strand_;

    std::array<char, 1024> buffer_;
    Sds011Parser sds011Parser;
    std::string port_;
    unsigned int baud_;
    EventBus& bus_;
};

#endif // SDS011_H
