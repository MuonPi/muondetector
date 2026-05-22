#include "hardware/ublox/serialublox.h"

#include "core/component.h"
#include "core/event_bus.h"
#include "core/logging/logger.h"
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
#include "data/commands/ubx_version_dependent_cmd.h"
#include "data/events/datastore_store_event.h"
#include "data/events/ubx_event.h"
#include "data/ublox/ublox_messages.h"
#include "data/ublox/ublox_structs.h"
#include "hardware/ublox/message_processor.h"
#include "hardware/ublox/ubx_parser.h"
#include "utility/helper_functions.h"

#include <array>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/syscall.h>
#include <termios.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

SerialUblox::SerialUblox(ComponentId id, boost::asio::io_context& io, const std::string& port,
                         unsigned int baud, UbxDynamicModel gnss_dynamic_model, EventBus& bus)
    : Component(id)
    , serial_(io)
    , timer_(io)
    , tx_strand_(boost::asio::make_strand(io))
    , rx_strand_(boost::asio::make_strand(io))
    , protocol_strand_(boost::asio::make_strand(io))
    , port_(port)
    , baud_(baud)
    , default_gnss_dynamic_model{gnss_dynamic_model}
    , bus_(bus) {
    makeConnection();

    // Command handling
    auto subscribeOnStrand = [this]<typename T>() {
        bus_.subscribe<T>([this](const T& cmd) {
            boost::asio::post(protocol_strand_, [this, cmd]() { handle(cmd); });
        });
    };

    bus_.subscribe<DatastoreStoreEvent<UbxAckAck>>(
        [this](const DatastoreStoreEvent<UbxAckAck>& event) {
            boost::asio::post(protocol_strand_, [this, &event]() { handle(event.data); });
        });

    bus_.subscribe<DatastoreStoreEvent<UbxAckNak>>(
        [this](const DatastoreStoreEvent<UbxAckNak>& event) {
            boost::asio::post(protocol_strand_, [this, &event]() { handle(event.data); });
        });

    subscribeOnStrand.template operator()<UbxDynamicModelCmd>();
    subscribeOnStrand.template operator()<UbxGnssConfigCmd>();
    subscribeOnStrand.template operator()<UbxMinCnoCmd>();
    subscribeOnStrand.template operator()<UbxMinMaxSvCmd>();
    subscribeOnStrand.template operator()<UbxMsgPollCmd>();
    subscribeOnStrand.template operator()<UbxMsgPollRateCmd>();
    // subscribeOnStrand.template operator()<UbxMsgRateCmd>();
    // subscribeOnStrand.template operator()<UbxRateCmd>();
    subscribeOnStrand.template operator()<UbxResetCmd>();
    subscribeOnStrand.template operator()<UbxSaveCmd>();
    subscribeOnStrand.template operator()<UbxSetAopCmd>();
    subscribeOnStrand.template operator()<UbxVersionDependentCmd>();
    subscribeOnStrand.template operator()<UbxTp5Cmd>();

    subscribeOnStrand.template operator()<UbxConfigDefaultCmd>();

    // Internally used events
    subscribeOnStrand.template operator()<GpsVersion>();
}

void SerialUblox::makeConnection() {
    boost::system::error_code ec;

    serial_.open(port_, ec);
    if (ec) {
        throw std::runtime_error(ec.message());
    }

    serial_.set_option(boost::asio::serial_port_base::baud_rate(baud_));

    int fd = serial_.native_handle();
    tcflush(fd, TCIOFLUSH);

    startAsyncRead();
    enqueueMessage(protSelCmd());
}

void SerialUblox::startAsyncRead() {
    serial_.async_read_some(
        boost::asio::buffer(buffer_),
        boost::asio::bind_executor(rx_strand_, [this](boost::system::error_code ec,
                                                      std::size_t length) {
            if (ec) {
                handleError("read", ec);
                retryLater();
                return;
            }
            ubxParser.feed(
                reinterpret_cast<const std::uint8_t*>(buffer_.data()), length,
                [this](UbxParser::UbxMessageView&& msg) {
                    UbxMessage m{msg.msg_id,
                                 std::string(reinterpret_cast<const char*>(msg.payload), msg.len)};

                    if (auto parsed = MessageProcessor::processMessage(m)) {
                        std::visit(
                            [this](auto&& v) { bus_.publish(DatastoreStoreEvent{.data = v}); },
                            *parsed);
                    }
                });
            startAsyncRead(); // continue reading
        }));
}

void SerialUblox::enqueueMessage(const UbxMessage& msg, bool trackAck) {
    bool is_allowed =
        init_state_ == InitState::Ready ||
        (init_state_ == InitState::Connecting && msg.full_id() == UBX_MSG::CFG_PRT) ||
        (init_state_ == InitState::ReceivedProtoAck && msg.full_id() == UBX_MSG::CFG_RATE);
    if (is_allowed) {
        send(msg, trackAck);
        return;
    } else {
        logError("Tried to queue message " + std::to_string(msg.full_id()) +
                 " but is not allowed at this state!");
    }
}

void SerialUblox::send(const UbxMessage& msg, bool trackAck) {
    if (trackAck) {
        pending_.emplace(msg.full_id(),
                         PendingMsg{.msg = msg, .last_send = std::chrono::steady_clock::now()});
    }
    logInfo("Send message " + std::to_string(msg.full_id()));
    boost::asio::post(tx_strand_, [this, data = msg.raw_message_string()]() {
        bool write_in_progress = !tx_queue_.empty();

        tx_queue_.push(data);

        if (!write_in_progress) {
            do_write();
        }
    });
}

void SerialUblox::do_write() {
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

void SerialUblox::handle(const UbxAckAck& ack) {
    logInfo("Got AckAck " + std::to_string(ack.msgID));

    boost::asio::post(protocol_strand_, [this, ack]() {
        auto range = pending_.equal_range(ack.msgID);
        if (range.first == range.second) {
            // Got acknowledge message from ublox without requesting anything
            const auto nameIt = UBX_MSG::msg_string.find(static_cast<UBX_MSG::msg_id>(ack.msgID));
            const std::string msgName =
                nameIt != UBX_MSG::msg_string.end() ? nameIt->second : "unknown";
            std::stringstream sstr{};
            sstr << "received unexpected " << "UBX-ACK-ACK"
                 << " message about " << msgName << " (msgID: 0x" << std::hex << ack.msgID;
            logWarn(sstr.str());
        } else {
            pending_.erase(range.first); // removes ONE element
        }

        if (init_state_ == InitState::Ready) {
            return;
        }

        if (ack.msgID == UBX_MSG::CFG_PRT) {
            init_state_ = InitState::ReceivedProtoAck;
            bus_.publish(UbxConfigDefaultCmd{});
            return;
        }
        if (ack.msgID == UBX_MSG::CFG_RATE) {
            init_state_ = InitState::ReceivedRateAck;
            bus_.publish(UbxConfigDefaultCmd{});
        }
    });
}

void SerialUblox::handle(const UbxAckNak& ackNak) {
    logInfo("Got AckNak " + std::to_string(ackNak.msgID));

    bus_.publish(DatastoreStoreEvent{CfgMsg{ackNak.msgID, -1}});

    boost::asio::post(protocol_strand_, [this, ackNak]() {
        auto range = pending_.equal_range(ackNak.msgID);
        if (range.first == range.second) {
            // Got acknowledge message from ublox without requesting anything
            const auto nameIt =
                UBX_MSG::msg_string.find(static_cast<UBX_MSG::msg_id>(ackNak.msgID));
            const std::string msgName =
                nameIt != UBX_MSG::msg_string.end() ? nameIt->second : "unknown";
            std::stringstream sstr{};
            sstr << "received unexpected " << "UBX-ACK-NAK"
                 << " message about " << msgName << " (msgID: 0x" << std::hex << ackNak.msgID;
            logWarn(sstr.str());
        } else {
            pending_.erase(range.first); // removes ONE element
        }
    });
}

void SerialUblox::handle(const UbxMsgRateCmd& cmd) {
    // set message rate on port. (rate 1 means every intervall the messages is sent)
    // if port = -1 set all ports
    logDebug("Processing UbxMsgRateCmd for message " + std::to_string(cmd.id));

    std::array<std::uint8_t, 8> data{};
    if (cmd.port > 6) {
        logWarn("port > 6 is not possible, port: " + std::to_string(cmd.port));
    }
    data[0] = static_cast<std::uint8_t>((cmd.id >> 8) & 0xff);
    data[1] = cmd.id & 0xff;
    if (cmd.port != 6) {
        data[2 + cmd.port] = cmd.rate;
    } else {
        for (int i = 2; i < 6; i++) {
            data[i] = cmd.rate;
        }
    }
    UbxMessage msg{UBX_MSG::CFG_MSG,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    enqueueMessage(std::move(msg));
    bus_.publish(UbxMsgPollRateCmd{cmd.id});
}

void SerialUblox::handle(const UbxMsgPollCmd& cmd) {
    logDebug("Processing UbxMsgPollCmd for message " + std::to_string(cmd.id));
    UbxMessage msg;
    std::array<std::uint8_t, 1> data{};
    if (cmd.id) {
        switch (cmd.id) {
            case UBX_MSG::CFG_PRT: // CFG-PRT
                // in this special case "rate" is the port ID
                data[0] = 1;
                enqueueMessage(
                    UbxMessage{cmd.id, std::string(reinterpret_cast<const char*>(data.data()),
                                                   data.size())},
                    false);
                break;
            case UBX_MSG::MON_VER:
                // the VER message apparently does not confirm reception with an ACK
                enqueueMessage(UbxMessage{cmd.id, ""}, false);
                break;
            case UBX_MSG::CFG_TP5:
                data[0] = 0;
                enqueueMessage(
                    UbxMessage{cmd.id, std::string(reinterpret_cast<const char*>(data.data()),
                                                   data.size())},
                    false);
                break;
            default:
                // for most messages the poll msg is just the message without payload
                enqueueMessage(UbxMessage{cmd.id, ""},
                               idsExpectedAck.contains(cmd.id) ? true : false);
                break;
        }
    }
}

void SerialUblox::handle(const UbxMsgPollRateCmd& cmd) {
    logDebug("Processing UbxMsgPollRateCmd for message " + std::to_string(cmd.id));
    std::array<std::uint8_t, 2> data{};
    data[0] = static_cast<std::uint8_t>((cmd.id >> 8) & 0xff);
    data[1] = static_cast<std::uint8_t>(cmd.id & 0xff);
    UbxMessage msg{UBX_MSG::CFG_MSG,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    enqueueMessage(std::move(msg));
}

void SerialUblox::handle(const UbxSetAopCmd& cmd) {
    logDebug("Processing UbxSetAopCmd");
    std::array<std::uint8_t, 40> data{};
    data[2] = 0x00; // aop flag in mask 2
    data[3] = 0x40; // all other flags are zero
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    data[27] = static_cast<std::uint8_t>(cmd.enable);
    data[30] = cmd.maxOrbErr & 0xff;
    data[31] = (cmd.maxOrbErr >> 8) & 0xff;

    UbxMessage msg{UBX_MSG::CFG_NAVX5,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    enqueueMessage(std::move(msg));
    bus_.publish(UbxMsgPollCmd{UBX_MSG::CFG_NAVX5});
}

void SerialUblox::handle(const UbxMinMaxSvCmd& cmd) {
    logDebug("Processing UbxMinMaxSvCmd");
    std::array<std::uint8_t, 40> data{};
    data[2] = 0x04; // MinMax flag in mask 1
    data[3] = 0x00; // all other flags are zero
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    data[10] = cmd.minSVs;
    data[11] = cmd.maxSVs;

    UbxMessage msg{UBX_MSG::CFG_NAVX5,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    enqueueMessage(std::move(msg));
    bus_.publish(UbxMsgPollCmd{UBX_MSG::CFG_NAVX5});
}

void SerialUblox::handle(const UbxMinCnoCmd& cmd) {
    logDebug("Processing UbxMinCnoCmd");
    std::array<std::uint8_t, 40> data{};
    data[2] = 0x08; // minCNO flag in mask 1
    data[3] = 0x00; // all other flags are zero
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    data[12] = cmd.minCNO;

    UbxMessage msg{UBX_MSG::CFG_NAVX5,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    enqueueMessage(std::move(msg));
    bus_.publish(UbxMsgPollCmd{UBX_MSG::CFG_NAVX5});
}

void SerialUblox::handle(const UbxSaveCmd& cmd) {
    logDebug("Processing UbxSaveCmd");
    std::array<std::uint8_t, 13> data{};
    // select the following sections to save:
    // ioPort, msgCfg, navCfg, rxmCfg, antConf
    uint32_t sectionMask = 0x01 | 0x02 | 0x08 | 0x10 | 0x400;

    data[0] = data[1] = data[2] = data[3] = 0x00;   // clear mask is all zero
    data[8] = data[9] = data[10] = data[11] = 0x00; // load mask is all zero
    data[4] = sectionMask & 0xff;
    data[5] = (sectionMask >> 8) & 0xff;
    data[6] = (sectionMask >> 16) & 0xff;
    data[7] = (sectionMask >> 24) & 0xff;
    data[12] = cmd.devMask;

    UbxMessage msg{UBX_MSG::CFG_CFG,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    enqueueMessage(std::move(msg));
}

void SerialUblox::handle(const UbxResetCmd& cmd) {
    logDebug("Processing UbxResetCmd");
    uint16_t navBbrMask = static_cast<std::uint16_t>((cmd.resetFlags >> 16) & 0xffff);
    uint8_t resetMode = static_cast<std::uint8_t>(cmd.resetFlags & 0xff);
    std::array<std::uint8_t, 4> data{};
    data[0] = static_cast<std::uint8_t>(navBbrMask & 0xff);
    data[1] = static_cast<std::uint8_t>((navBbrMask >> 8) & 0xff);
    data[2] = resetMode;
    data[3] = 0;

    UbxMessage msg{UBX_MSG::CFG_RST,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    enqueueMessage(std::move(msg));
}

void SerialUblox::processQueuedCmds() {
    if (protocolVersion_.has_value() == false) {
        // If no protocol version was polled yet, poll it now!
        // (And with now we always mean asynchronously through event queue)
        bus_.publish(UbxMsgPollCmd{UBX_MSG::MON_VER});
        return;
    }
    // If protocol was received, go through stored version dependent commands
    std::string versionStr = protocolVersion_.value().prot;
    std::optional<UbxProtVersion> version = MessageProcessor::getProtVersion(versionStr);
    if (version.has_value() == false) {
        logError("Could not parse version string " + versionStr);
        return;
    }
    std::size_t counter{0};
    while (queuedCmds_.empty() == false) {
        if (counter % 6 == 5) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        auto command = std::move(queuedCmds_.front());
        queuedCmds_.pop();
        for (auto entry : command.config) {
            if (UbxVersionDependentCmd::applies(entry, version.value())) {
                std::visit([&](const auto& value) { handle(value); }, entry.cmd);
            }
        }
    }
}

void SerialUblox::handle(const UbxDynamicModelCmd& cmd) {
    std::string buf;
    buf.resize(36);
    // unsigned char* buf { static_cast<unsigned char*>(calloc(sizeof(unsigned char), 36)) };
    buf[0] = 0x01;
    buf[1] = 0x00;
    buf[2] = static_cast<std::uint8_t>(cmd.model); // dyn Model

    logInfo("setting GNSS dynamic model to " + std::to_string(static_cast<unsigned>(cmd.model)));

    enqueueMessage(UbxMessage{UBX_MSG::CFG_NAV5, buf});
    bus_.publish(UbxMsgPollCmd{UBX_MSG::CFG_NAV5});
}

void SerialUblox::handle(const UbxGnssConfigCmd& cmd) {
    // If protocol version is not clear yet, delay processing
    if (protocolVersion_.has_value() == false) {
        queuedCmds_.emplace(
            UbxVersionDependentCmd{.config{{UbxProtVersion{0, 1}, UbxProtVersion{99, 0}, cmd}}});
        processQueuedCmds();
        return;
    }

    std::string versionStr = protocolVersion_.value().prot;
    std::optional<UbxProtVersion> version = MessageProcessor::getProtVersion(versionStr);

    const std::size_t N = cmd.gnssConfigs.size();
    std::string data;
    data.resize(4 + 8 * N);
    // unsigned char* data { static_cast<unsigned char*>(calloc(sizeof(unsigned char), 4 + 8 * N))
    // };
    data[0] = 0;
    data[1] = 0;
    data[2] = 0xff;
    data[3] = static_cast<std::uint8_t>(N);

    for (std::size_t i{0}; i < N; i++) {
        uint32_t flags = cmd.gnssConfigs[i].flags;
        if (version.value().major >= 15.0) {
            flags |= 0x0001 << 16;
            if (cmd.gnssConfigs[i].gnssId == 5)
                flags |= 0x0004 << 16;
        }
        data[4 + 8 * i] = cmd.gnssConfigs[i].gnssId;
        data[5 + 8 * i] = cmd.gnssConfigs[i].resTrkCh;
        data[6 + 8 * i] = cmd.gnssConfigs[i].maxTrkCh;
        data[8 + 8 * i] = flags & 0xff;
        data[9 + 8 * i] = (flags >> 8) & 0xff;
        data[10 + 8 * i] = (flags >> 16) & 0xff;
        data[11 + 8 * i] = (flags >> 24) & 0xff;
    }

    enqueueMessage(UbxMessage{UBX_MSG::CFG_GNSS, data});
    bus_.publish(UbxMsgPollCmd{UBX_MSG::CFG_GNSS});
}

void SerialUblox::handle(const UbxTp5Cmd& tp) {

    std::array<std::uint8_t, 32> buf{0};
    put<std::uint8_t>(buf.begin(), tp.tpIndex);
    put<std::uint8_t>(buf.begin() + 1, 0);
    put<std::uint16_t>(buf.begin() + 4, tp.antCableDelay);
    put<std::uint16_t>(buf.begin() + 6, tp.rfGroupDelay);
    put<std::uint32_t>(buf.begin() + 8, tp.freqPeriod);
    put<std::uint32_t>(buf.begin() + 12, tp.freqPeriodLock);
    put<std::uint32_t>(buf.begin() + 16, tp.pulseLenRatio);
    put<std::uint32_t>(buf.begin() + 20, tp.pulseLenRatioLock);
    put<std::uint32_t>(buf.begin() + 24, tp.userConfigDelay);
    put<std::uint32_t>(buf.begin() + 28, tp.flags);
    enqueueMessage(UbxMessage{UBX_MSG::CFG_TP5, std::string(buf.begin(), buf.end())});
    bus_.publish(UbxMsgPollCmd{UBX_MSG::CFG_TP5});
}

void SerialUblox::handle(const UbxVersionDependentCmd& cmd) {
    logDebug("Processing UbxVersionDependentCmd");
    queuedCmds_.emplace(cmd);
    processQueuedCmds();
}

void SerialUblox::handle(const GpsVersion& protocolVersion) {
    if (protocolVersion_.has_value()) {
        return;
    }
    protocolVersion_ = protocolVersion;
    processQueuedCmds();
}

void SerialUblox::handle([[maybe_unused]] const UbxConfigDefaultCmd&) {
    if (init_state_ == InitState::Connecting) {
        logInfo("Called UbxConfigDefaultCmd sending prot sel cmd");
        enqueueMessage(protSelCmd());
    }
    if (init_state_ == InitState::ReceivedProtoAck) {
        logInfo("Called UbxConfigDefaultCmd sending rate sel cmd");
        enqueueMessage(rateSelCmd());
    }
    if (init_state_ == InitState::ReceivedRateAck) {
        init_state_ = InitState::Ready;
        logInfo("Called UbxConfigDefaultCmd sending setiü");

        handle(UbxDynamicModelCmd{default_gnss_dynamic_model});
        handle(UbxSetAopCmd{true});
        // --- Message Rates ---
        initAllUbxMsgRate();
        pollAllUbxMsg();
        pollAllUbxMsgRate();
        // Enable NAV_SVINFO only for version 0.1 - 15.0
        // Enable NAV_SAT for version > 15.0
        handle(UbxVersionDependentCmd{
            {UbxVersionDependentCmd::Entry{UbxProtVersion{0, 1}, UbxProtVersion{15, 0},
                                           UbxMsgRateCmd{UBX_MSG::NAV_SVINFO, 1, 69}},
             UbxVersionDependentCmd::Entry{UbxProtVersion{15, 0},
                                           UbxProtVersion{std::numeric_limits<unsigned>().max(), 0},
                                           UbxMsgRateCmd{UBX_MSG::NAV_SVINFO, 1, 0}}}});
        handle(UbxVersionDependentCmd{{UbxVersionDependentCmd::Entry{
            UbxProtVersion{15, 0}, UbxProtVersion{std::numeric_limits<unsigned>().max(), 0},
            UbxMsgRateCmd{UBX_MSG::NAV_SAT, 1, 69}}}});
    }
}

void SerialUblox::initAllUbxMsgRate() {
    for (auto& [msgID, data] : defaultRates) {
        handle(UbxMsgRateCmd{msgID, data.first, data.second});
    }
}

void SerialUblox::pollAllUbxMsgRate() {
    for (auto msgID : rateCfgMsgID) {
        handle(UbxMsgPollRateCmd{msgID});
    }
}

void SerialUblox::pollAllUbxMsg() {
    handle(UbxMsgPollCmd{UBX_MSG::MON_VER});
    handle(UbxMsgPollCmd{UBX_MSG::CFG_GNSS});
    handle(UbxMsgPollCmd{UBX_MSG::CFG_TP5});
    // Rate configurable:
    for (auto msgID : rateCfgMsgID) {
        handle(UbxMsgPollCmd{msgID});
    }
}

auto SerialUblox::protSelCmd() -> UbxMessage {
    std::array<std::uint8_t, 20> data{};
    if (uart_port == 1) {
        // uart_port 1 is UART port, payload for other ports may differ
        // setup payload. Bit masks are set up byte wise from lowest to highest byte
        data[0] = uart_port; // set port (1 Byte)
        data[1] = 0;         // reserved
        data[2] = 0;
        data[3] = 0; // txReady options (not used)
        // mode option:
        data[4] = 0b11000000; // charLen option (first 2 bits): 11 means 8 bit character length.
                              // (10 means 7 bit character length only with parity enabled)
        data[5] = 0b00001000; // first 2 bits unimportant. 00 -> 1 stop bit. 100 -> no parity. last
                              // bit unimportant.
        data[6] = 0;
        data[7] = 0; // part of mode option but no meaning

        // baudrate: (check if it works)
        data[8] = static_cast<std::uint8_t>((static_cast<std::uint32_t>(baud_)) & 0xff);
        data[9] = static_cast<std::uint8_t>((static_cast<std::uint32_t>(baud_) >> 8) & 0xff);
        data[10] = static_cast<std::uint8_t>((static_cast<std::uint32_t>(baud_) >> 16) & 0xff);
        data[11] = 0; // not needed because baudRate will never be over 16777216 (2^24)

        // inProtoMask enables/disables possible protocols for sending messages to the gps module:
        data[12] = 0b00100111; // () () (RTCM3) () () (RTCM) (NMEA) (UBX)
        data[13] = 0;          // has no meaning but part of inProtoMask

        // outProtoMask enables/disables protocols for receiving messages from the gps module:
        data[14] = outProtocolMask;

        data[15] = 0; // has no meaning but part of outProtoMask

        data[16] = 0;
        data[17] = 0; // extendedTxTimeout not needed
        data[18] = 0;
        data[19] = 0; // reserved
    }
    logDebug("UbxprotocolSelectionCmd port: " + std::to_string(uart_port) +
             " mask: " + std::to_string(outProtocolMask));
    UbxMessage msg{UBX_MSG::CFG_PRT,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    return msg;
}

auto SerialUblox::rateSelCmd() -> UbxMessage {
    constexpr std::uint16_t MinMeasRate = 10;
    constexpr std::uint16_t MinNavRate = 1;
    constexpr std::uint16_t MaxNavRate = 127;

    if (measRate < MinMeasRate || navRate < MinNavRate || navRate > MaxNavRate) {
        throw std::runtime_error("Invalid UBX rate command: measRate={}, navRate={}" +
                                 std::to_string(measRate) + std::to_string(navRate));
    }

    std::array<std::uint8_t, 6> data{
        static_cast<std::uint8_t>(measRate & 0xff),
        static_cast<std::uint8_t>((measRate >> 8) & 0xff),
        static_cast<std::uint8_t>(navRate & 0xff),
        static_cast<std::uint8_t>((navRate >> 8) & 0xff),
        0x00, // timeRef LSB
        0x00  // timeRef MSB
    };

    UbxMessage msg{UBX_MSG::CFG_RATE,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    return msg;
}
