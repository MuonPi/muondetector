#ifndef UBLOX_STRUCTS_H
#define UBLOX_STRUCTS_H

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <variant>
#include <optional>
#include <vector>

namespace Gnss {


struct Id {
    enum {
        GPS = 0,
        SBAS = 1,
        GAL = 2,
        BEID = 3,
        IMES = 4,
        QZSS = 5,
        GLNS = 6,
        Undefined = 7,
        first = GPS,
        last = Undefined
    };
    static constexpr std::array<const char*, last + 1> name { "GPS", "SBAS", "GAL", "BEID", "IMES", "QZSS", "GLNS", "N/A" };
};

struct FixType {
    enum {
        None = 0,
        DeadReckoning = 1,
        Fix2d = 2,
        Fix3d = 3,
        GpsDeadReckoning = 4,
        TimeFix = 5,
        Undefined = 6,
        first = None,
        last = Undefined
    };
    size_t value { None };
    static constexpr std::array<const char*, last + 1> name { "NoFix", "DeadReck", "2D", "3D", "GPS+DeadReck", "TimeFix", "N/A" };
};

struct OrbitSource {
    enum {
        None = 0,
        Ephem = 1,
        Almanac = 2,
        Aop = 3,
        Aop2 = 4,
        Alt1 = 5,
        Alt2 = 6,
        Alt3 = 7,
        Undefined = 8,
        first = None,
        last = Undefined
    };
    static constexpr std::array<const char*, last + 1> name { "N/A", "Ephem", "Alm", "AOP", "AOP+", "Alt", "Alt", "Alt", "Undef" };
};

struct AntennaStatus {
    enum {
        Init = 0,
        Unknown1 = 1,
        Ok = 2,
        ShortCircuit = 3,
        Open = 4,
        Unknown2 = 5,
        Unknown3 = 6,
        Undefined = 7,
        first = Init,
        last = Undefined
    };
    static constexpr std::array<const char*, last + 1> name { "init", "unknown", "ok", "short", "open", "unknown", "unknown", "N/A" };
};

struct SvHealth {
    enum {
        Undefined = 0,
        Good = 1,
        Bad = 2,
        VeryBad = 3,
        first = Undefined,
        last = VeryBad
    };
    static constexpr std::array<const char*, last + 1> name { "N/A", "good", "bad", "bad+" };
};

} // namespace Gnss


constexpr std::size_t s_nr_targets { 6 };
constexpr std::size_t s_default_target { 1 };

struct UbxMessage {
    UbxMessage() = default;
    UbxMessage(std::uint16_t msg_id, const std::string a_payload) noexcept;

    [[nodiscard]] auto full_id() const -> std::uint16_t;
    [[nodiscard]] auto payload() const -> const std::string&;
    [[nodiscard]] auto class_id() const -> std::uint8_t;
    [[nodiscard]] auto message_id() const -> std::uint8_t;
    [[nodiscard]] auto raw_message_string() const -> std::string;
    [[nodiscard]] auto check_sum() const -> std::uint16_t;
    [[nodiscard]] static auto check_sum(const std::string& data) -> std::uint16_t;

private:
    std::uint16_t m_full_id { 0 };
    std::string m_payload {};
};


struct GnssConfigStruct {
    std::uint8_t gnssId;
    std::uint8_t resTrkCh;
    std::uint8_t maxTrkCh;
    std::uint32_t flags;
};


#endif // UBLOX_STRUCTS_H
