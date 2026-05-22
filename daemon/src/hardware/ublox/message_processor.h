#ifndef MESSAGE_PROCESSOR_H
#define MESSAGE_PROCESSOR_H

#include "data/events/ubx_event.h"
#include "data/ublox/ublox_messages.h"

#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

class MessageProcessor {
  public:
    MessageProcessor() = delete;
    ~MessageProcessor() = delete;

    // all functions only used for processing and showing "UbxMessage"
    static auto processMessage(const UbxMessage& msg) -> std::optional<UbxEvent>;

    static const std::unordered_map<
        UBX_MSG::msg_id, std::pair<std::function<std::optional<UbxEvent>()>, std::string>>
        handler;

    static auto getProtVersion(std::string_view text) -> std::optional<UbxProtVersion>;

  private:
    static const std::unordered_map<std::uint8_t, const char*> ubx_class_names;
    static const std::unordered_map<
        std::uint16_t, std::pair<std::optional<UbxEvent> (*)(const std::string&), std::string>>
        handlerLookup;
    static auto UBXAck(const UbxMessage& msg) -> std::optional<UbxEvent>;
    static void unhandled(const UbxMessage& msg);
    static auto UBXNavStatus(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXNavDOP(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXNavTimeGPS(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXNavTimeUTC(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXNavClock(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXNavSVinfo(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXNavSat(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXNavPosLLH(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXCfgAnt(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXCfgNavX5(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXCfgNav5(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXCfgTP5(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXCfgGNSS(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXCfgMSG(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXMonRx(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXMonTx(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXMonHW(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXMonHW2(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXMonVer(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXTimTP(const std::string& msg) -> std::optional<UbxEvent>;
    static auto UBXTimTM2(const std::string& msg) -> std::optional<UbxEvent>;
    static std::optional<GpsVersion> gpsVersion;
};

#endif // MESSAGE_PROCESSOR_H
