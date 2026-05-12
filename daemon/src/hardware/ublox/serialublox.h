#ifndef SERIALUBLOX_H
#define SERIALUBLOX_H

#include "core/component.h"
#include "core/event_bus.h"
#include "data/commands/ubx_min_cno_cmd.h"
#include "data/commands/ubx_min_max_sv_cmd.h"
#include "data/commands/ubx_msg_poll_cmd.h"
#include "data/commands/ubx_msg_poll_rate_cmd.h"
#include "data/commands/ubx_msg_rate_cmd.h"
#include "data/commands/ubx_protocol_selection_cmd.h"
#include "data/commands/ubx_rate_cmd.h"
#include "data/commands/ubx_reset_cmd.h"
#include "data/commands/ubx_save_cmd.h"
#include "data/commands/ubx_set_aop_cmd.h"
#include "data/commands/ubx_version_dependent_msg_rate_cmd.h"
#include "data/events/ubx_event.h"
#include "data/ublox/ublox_structs.h"

#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <unordered_map>

struct GnssPosStruct;
struct GnssMonHwStruct;
struct GnssMonHw2Struct;
struct UbxTimeMarkStruct;

class SerialUblox : public Component {
  public:
    SerialUblox(ComponentId id, boost::asio::io_context& io, const std::string& port,
                unsigned int baud, EventBus& bus);

    void makeConnection();
    auto enqueueMessage(const UbxMessage& msg) -> bool;

  private:
    void startAsyncRead();

    void handle(const UbxRateCmd&);
    void handle(const UbxMsgRateCmd&);
    void handle(const UbxMsgPollCmd&);
    void handle(const UbxMsgPollRateCmd&);
    void handle(const UbxProtocolSelectionCmd&);
    void handle(const UbxSaveCmd&);
    void handle(const UbxResetCmd&);
    void handle(const UbxSetAopCmd&);
    void handle(const UbxMinMaxSvCmd&);
    void handle(const UbxMinCnoCmd&);
    void handle(const UbxVersionDependentMsgRateCmd&);
    void handle(const GpsVersion&);

    void processQueuedCmds();

    void retryLater() {
        timer_.expires_after(std::chrono::seconds(timeout_));
        timer_.async_wait([this](boost::system::error_code) { makeConnection(); });
    }

    void handleError(const std::string& where, const boost::system::error_code& ec) {
        std::cerr << "Error in " << where << ": " << ec.message() << std::endl;
    }

    auto parseStreamForMsg(std::string& buffer) -> std::optional<UbxMessage>;

    void do_write();

    boost::asio::serial_port serial_;
    boost::asio::steady_timer timer_;
    std::size_t maxQueueSize_ = 3000;
    std::queue<std::string> tx_queue_;

    std::array<char, 1024> buffer_;
    std::string m_buffer = "";

    boost::asio::strand<boost::asio::io_context::executor_type> strand_;
    std::string port_;
    unsigned int baud_;
    EventBus& bus_;
    int timeout_ = 5;
    std::optional<GpsVersion> protocolVersion_;
    std::queue<UbxVersionDependentMsgRateCmd> queuedCmds_;

    // TODO: Move this to some separate processor/storage
    std::size_t waitingForAppliedMsgRate{0};
    std::unordered_map<uint16_t, int> msgRateCfgs;
};

#endif // SERIALUBLOX_H
