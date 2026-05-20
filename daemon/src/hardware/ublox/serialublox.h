#ifndef SERIALUBLOX_H
#define SERIALUBLOX_H

#include "core/component.h"
#include "core/event_bus.h"
#include "data/commands/ubx_config_default_cmd.h"
#include "data/commands/ubx_dynamic_model_cmd.h"
#include "data/commands/ubx_gnss_config_cmd.h"
#include "data/commands/ubx_min_cno_cmd.h"
#include "data/commands/ubx_min_max_sv_cmd.h"
#include "data/commands/ubx_msg_poll_cmd.h"
#include "data/commands/ubx_msg_poll_rate_cmd.h"
#include "data/commands/ubx_msg_rate_cmd.h"
#include "data/commands/ubx_rate_cmd.h"
#include "data/commands/ubx_reset_cmd.h"
#include "data/commands/ubx_save_cmd.h"
#include "data/commands/ubx_set_aop_cmd.h"
#include "data/commands/ubx_tp5_cmd.h"
#include "data/commands/ubx_version_dependent_cmd.h"
#include "data/events/ubx_event.h"
#include "data/ublox/ublox_structs.h"

#include <boost/asio.hpp>
#include <future>
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
                unsigned int baud, UbxDynamicModel gnss_dynamic_model, EventBus& bus);

    std::uint8_t uart_port = 1;
    std::uint8_t outProtocolMask = PROTO_UBX;
    std::uint16_t measRate = 100;
    std::uint16_t navRate = 1;

  private:
    enum class InitState { Connecting, ReceivedProtoAck, ReceivedRateAck, Ready };

    // struct PendingMsg {
    //     UbxMessage msg;
    //     std::chrono::steady_clock::time_point last_send;
    // };
    // std::unordered_multimap<std::uint16_t, PendingMsg> pending_;
    std::atomic<InitState> init_state_{InitState::Connecting};

    void makeConnection();
    void startAsyncRead();
    void send(const UbxMessage& msg, bool trackAck = true);
    void enqueueMessage(const UbxMessage& msg, bool trackAck = true);

    void initAllUbxMsgRate();
    void pollAllUbxMsgRate();
    void pollAllUbxMsg();
    auto protSelCmd() -> UbxMessage;
    auto rateSelCmd() -> UbxMessage;

    void handle(const UbxDynamicModelCmd&);
    void handle(const UbxGnssConfigCmd&);
    void handle(const UbxMinCnoCmd&);
    void handle(const UbxMinMaxSvCmd&);
    void handle(const UbxMsgPollCmd&);
    void handle(const UbxMsgPollRateCmd&);
    void handle(const UbxMsgRateCmd&);
    void handle(const UbxResetCmd&);
    void handle(const UbxSaveCmd&);
    void handle(const UbxSetAopCmd&);
    void handle(const UbxTp5Cmd&); // Set time pulse config (TP5)
    void handle(const UbxVersionDependentCmd&);
    void handle(const UbxConfigDefaultCmd&);

    void handle(const GpsVersion&);

    void initCmdHandlers();
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

    // void flushDeferredQueue();

    void handleAck(const UbxAckAck& ack);
    void handleNak(const UbxAckNak& ackNak);

    boost::asio::serial_port serial_;
    boost::asio::steady_timer timer_;
    std::queue<std::string> tx_queue_;

    std::array<char, 1024> buffer_;
    std::string m_buffer = "";

    // boost::asio::strand<boost::asio::io_context::executor_type> tx_strand_;
    boost::asio::strand<boost::asio::io_context::executor_type> rx_strand_;
    // boost::asio::strand<boost::asio::io_context::executor_type> protocol_strand_;
    std::string port_;
    unsigned int baud_;
    UbxDynamicModel default_gnss_dynamic_model;
    EventBus& bus_;
    int timeout_ = 5;
    std::optional<GpsVersion> protocolVersion_;
    // std::queue<UbxMessage> deferredCmds_;
    std::queue<UbxVersionDependentCmd> queuedCmds_;
};

#endif // SERIALUBLOX_H
