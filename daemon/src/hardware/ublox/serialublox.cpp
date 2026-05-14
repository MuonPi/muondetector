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
#include "data/commands/ubx_protocol_selection_cmd.h"
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
#include "utility/helper_functions.h"

#include <array>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/syscall.h>
#include <unistd.h>
#include <vector>

SerialUblox::SerialUblox(ComponentId id, boost::asio::io_context& io, const std::string& port,
                         unsigned int baud, EventBus& bus)
    : Component(id)
    , serial_(io)
    , timer_(io)
    , strand_(boost::asio::make_strand(io))
    , port_(port)
    , baud_(baud)
    , bus_(bus) {
    makeConnection();

    // Command handling
    bus_.subscribe<UbxDynamicModelCmd>([this](const UbxDynamicModelCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxGnssConfigCmd>([this](const UbxGnssConfigCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxMinCnoCmd>([this](const UbxMinCnoCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxMinMaxSvCmd>([this](const UbxMinMaxSvCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxMsgPollCmd>([this](const UbxMsgPollCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxMsgPollRateCmd>([this](const UbxMsgPollRateCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxMsgRateCmd>([this](const UbxMsgRateCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxRateCmd>([this](const UbxRateCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxProtocolSelectionCmd>(
        [this](const UbxProtocolSelectionCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxResetCmd>([this](const UbxResetCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxSaveCmd>([this](const UbxSaveCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxSetAopCmd>([this](const UbxSetAopCmd& cmd) { handle(cmd); });
    bus_.subscribe<UbxVersionDependentCmd>(
        [this](const UbxVersionDependentCmd& cmd) { handle(cmd); });
    bus.subscribe<UbxTp5Cmd>([this](const UbxTp5Cmd& cmd) { handle(cmd); });

    // Internally used events
    bus_.subscribe<GpsVersion>([this](const GpsVersion& gpsVersion) { handle(gpsVersion); });
    bus_.subscribe<UbxAckNak>(
        [this](const UbxAckNak& event) { msgRateCfgs.emplace(event.msgID, -1); });
}

void SerialUblox::makeConnection() {
    boost::system::error_code ec;

    serial_.open(port_, ec);
    if (ec) {
        throw std::runtime_error(ec.message());
    }

    serial_.set_option(boost::asio::serial_port_base::baud_rate(baud_));

    startAsyncRead();
}

void SerialUblox::startAsyncRead() {
    serial_.async_read_some(
        boost::asio::buffer(buffer_), [this](boost::system::error_code ec, std::size_t length) {
            if (ec) {
                handleError("read", ec);
                retryLater();
                return;
            }
            m_buffer += std::string(buffer_.data(), length);
            std::optional<UbxMessage> msgCandidate{std::nullopt};
            do {
                if ((msgCandidate = parseStreamForMsg(m_buffer))) {
                    if (auto parsed = MessageProcessor::processMessage(msgCandidate.value())) {
                        std::visit(
                            [this](auto&& msg) {
                                using T = std::decay_t<decltype(msg)>;
                                bus_.publish(DatastoreStoreEvent<T>{.data = msg}); // typed publish
                            },
                            parsed.value());
                    }
                }
            } while (msgCandidate.has_value());
            startAsyncRead(); // continue reading
        });
}

auto SerialUblox::parseStreamForMsg(std::string& buffer)
    -> std::optional<UbxMessage> { // gets the (maybe not finished) buffer and checks for messages
                                   // in it that make sense
    if (buffer.size() < 9) {
        return std::nullopt;
    }
    // refstr are the first two hex numbers defining the header of an ubx message
    const std::string refstr{static_cast<char>(0xb5), static_cast<char>(0x62)};
    std::size_t found = buffer.find(refstr);
    if (found == std::string::npos) {
        // discard everything before the start of a NMEA message, too
        // to ensure that buffer won't grow too big
        std::string beginNMEA = "$";
        found = 0;
        while (found != std::string::npos) {
            found = buffer.find(beginNMEA);
            if (found == std::string::npos) {
                break;
            }
            buffer.erase(0, found + 1);
        }

        return std::nullopt;
    }

    const std::string message_raw_data{buffer.substr(found, buffer.size())};
    // discard everything before start of the message
    buffer.erase(0, found);

    if (message_raw_data.size() < 8)
        return std::nullopt;
    std::uint16_t len = static_cast<std::uint16_t>(message_raw_data[4]);
    len |= static_cast<uint16_t>(message_raw_data[5]) << 8;
    if ((message_raw_data.size() - 8u) < len) {
        found = buffer.find(refstr, 2);
        if (found != std::string::npos) {
            if (logLevel() == LogLevel::Debug) {
                std::stringstream sstr;
                sstr << "received faulty UBX string:\n " << std::dec;
                for (std::string::size_type i = 0; i < found; i++) {
                    sstr << std::setw(2) << std::hex << "0x" << static_cast<unsigned>(buffer[i])
                         << " ";
                }
                sstr << std::endl;
                logInfo(sstr.str());
            }
            buffer.erase(0, found);
        }
        return std::nullopt;
    }
    buffer.erase(0, len + 8);

    std::uint16_t msg_id{
        static_cast<std::uint16_t>((message_raw_data[2] << 8) | (message_raw_data[3] & 0xff))};

    UbxMessage message{msg_id, message_raw_data.substr(6, len)};
    auto checksum{message.check_sum()};
    if ((checksum & 0xff) == message_raw_data[len + 6] &&
        (checksum >> 8) == message_raw_data[len + 7]) {
        return message;
    }
    return std::nullopt;
}

auto SerialUblox::enqueueMessage(const UbxMessage& msg) -> bool {
    boost::asio::post(strand_, [this, data = msg.raw_message_string()]() mutable {
        bool write_in_progress = !tx_queue_.empty();

        tx_queue_.push(std::move(data));

        if (!write_in_progress)
            do_write();
    });

    return true;
}

void SerialUblox::do_write() {
    boost::asio::async_write(
        serial_, boost::asio::buffer(tx_queue_.front()),
        boost::asio::bind_executor(strand_, [this](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                tx_queue_.pop();

                if (!tx_queue_.empty())
                    do_write();
            }
        }));
}

void SerialUblox::handle(const UbxRateCmd& cmd) {
    logDebug("Processing UbxRateCmd");

    constexpr std::uint16_t MinMeasRate = 10;
    constexpr std::uint16_t MinNavRate = 1;
    constexpr std::uint16_t MaxNavRate = 127;

    if (cmd.measRate < MinMeasRate || cmd.navRate < MinNavRate || cmd.navRate > MaxNavRate) {
        logWarn("Invalid UBX rate command: measRate={}, navRate={}" + std::to_string(cmd.measRate) +
                std::to_string(cmd.navRate));
        return;
    }

    std::array<std::uint8_t, 6> data{
        static_cast<std::uint8_t>(cmd.measRate & 0xff),
        static_cast<std::uint8_t>((cmd.measRate >> 8) & 0xff),
        static_cast<std::uint8_t>(cmd.navRate & 0xff),
        static_cast<std::uint8_t>((cmd.navRate >> 8) & 0xff),
        0x00, // timeRef LSB
        0x00  // timeRef MSB
    };

    UbxMessage msg{UBX_MSG::CFG_RATE,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    enqueueMessage(std::move(msg));
}

void SerialUblox::handle(const UbxMsgRateCmd& cmd) {
    // set message rate on port. (rate 1 means every intervall the messages is sent)
    // if port = -1 set all ports
    logDebug("Processing UbxMsgRateCmd for message " + std::to_string(cmd.id));

    std::array<std::uint8_t, 8> data;
    if (cmd.port > 6) {
        logWarn("port > 5 is not possible, port: " + std::to_string(cmd.port));
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
}

void SerialUblox::handle(const UbxMsgPollCmd& cmd) {
    logDebug("Processing UbxMsgPollCmd for message " + std::to_string(cmd.id));
    UbxMessage msg;
    std::array<std::uint8_t, 1> data;
    switch (cmd.id) {
        case UBX_MSG::CFG_PRT: // CFG-PRT
            // in this special case "rate" is the port ID
            data[0] = 1;
            enqueueMessage(UbxMessage{
                cmd.id, std::string(reinterpret_cast<const char*>(data.data()), data.size())});
            break;
        case UBX_MSG::MON_VER:
            // the VER message apparently does not confirm reception with an ACK
            enqueueMessage(UbxMessage{cmd.id, ""});
            break;
        default:
            // for most messages the poll msg is just the message without payload
            enqueueMessage(UbxMessage{cmd.id, ""});
            break;
    }
}

void SerialUblox::handle(const UbxMsgPollRateCmd& cmd) {
    logDebug("Processing UbxMsgPollRateCmd for message " + std::to_string(cmd.id));
    std::array<std::uint8_t, 2> data;
    data[0] = static_cast<std::uint8_t>((cmd.id >> 8) & 0xff);
    data[1] = static_cast<std::uint8_t>(cmd.id & 0xff);
    UbxMessage msg{UBX_MSG::CFG_MSG,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    enqueueMessage(std::move(msg));
}

void SerialUblox::handle(const UbxSetAopCmd& cmd) {
    logDebug("Processing UbxSetAopCmd");
    std::array<std::uint8_t, 40> data;
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
}

void SerialUblox::handle(const UbxMinMaxSvCmd& cmd) {
    logDebug("Processing UbxMinMaxSvCmd");
    std::array<std::uint8_t, 40> data;
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
}

void SerialUblox::handle(const UbxMinCnoCmd& cmd) {
    logDebug("Processing UbxMinCnoCmd");
    std::array<std::uint8_t, 40> data;
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
}

void SerialUblox::handle(const UbxProtocolSelectionCmd& cmd) {
    logDebug("Processing UbxProtocolSelectionCmd");

    if (cmd.port >= s_nr_targets) {
        logWarn("port > " + std::to_string(s_nr_targets - 1) + " is not possible");
        return;
    }

    std::array<std::uint8_t, 20> data;
    if (cmd.port == 1) {
        // port 1 is UART port, payload for other ports may differ
        // setup payload. Bit masks are set up byte wise from lowest to highest byte
        data[0] = cmd.port; // set port (1 Byte)
        data[1] = 0;        // reserved
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
        data[14] = cmd.outProtocolMask;

        data[15] = 0; // has no meaning but part of outProtoMask

        data[16] = 0;
        data[17] = 0; // extendedTxTimeout not needed
        data[18] = 0;
        data[19] = 0; // reserved
    }
    UbxMessage msg{UBX_MSG::CFG_PRT,
                   std::string(reinterpret_cast<const char*>(data.data()), data.size())};
    enqueueMessage(std::move(msg));
}

void SerialUblox::handle(const UbxSaveCmd& cmd) {
    logDebug("Processing UbxSaveCmd");
    std::array<std::uint8_t, 13> data;
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
    std::array<std::uint8_t, 4> data;
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
        return;
    }
    // If protocol was received, go through stored version dependent commands
    std::string versionStr = protocolVersion_.value().prot;
    std::optional<Version> version = MessageProcessor::getProtVersion(versionStr);
    if (version.has_value() == false) {
        logError("Could not parse version string " + versionStr);
        return;
    }
    while (queuedCmds_.empty() == false) {
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
    enqueueMessage(UbxMessage{UBX_MSG::CFG_NAV5, buf});
}

void SerialUblox::handle(const UbxGnssConfigCmd& cmd) {
    // If protocol version is not clear yet, delay processing
    if (protocolVersion_.has_value() == false) {
        queuedCmds_.emplace(UbxVersionDependentCmd{.config{{Version{0, 1}, Version{99, 0}, cmd}}});
        processQueuedCmds();
        return;
    }

    std::string versionStr = protocolVersion_.value().prot;
    std::optional<Version> version = MessageProcessor::getProtVersion(versionStr);

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