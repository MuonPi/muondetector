#ifndef UBLOX_STRUCTS_H
#define UBLOX_STRUCTS_H

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <variant>

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

class GnssSatellite {
public:
    GnssSatellite() = default;
    GnssSatellite(int gnssId, int satId, int cnr, int elev, int azim, float prRes, std::uint32_t flags)
        : GnssId(gnssId)
        , SatId(satId)
        , Cnr(cnr)
        , Elev(elev)
        , Azim(azim)
        , PrRes(prRes)
    {
        Quality = (int)(flags & 0x07);
        if (flags & 0x08)
            Used = true;
        else
            Used = false;
        Health = (int)(flags >> 4 & 0x03);
        OrbitSource = (flags >> 8 & 0x07);
        Smoothed = (flags & 0x80);
        DiffCorr = (flags & 0x40);
    }

    GnssSatellite(int gnssId, int satId, int cnr, int elev, int azim, float prRes,
        int quality, int health, int orbitSource, bool used, bool diffCorr, bool smoothed)
        : GnssId(gnssId)
        , SatId(satId)
        , Cnr(cnr)
        , Elev(elev)
        , Azim(azim)
        , PrRes(prRes)
        , Quality(quality)
        , Health(health)
        , OrbitSource(orbitSource)
        , Used(used)
        , DiffCorr(diffCorr)
        , Smoothed(smoothed)
    {
    }

    ~GnssSatellite() { }

    static void PrintHeader(bool wIndex);
    void Print(bool wHeader) const;
    void Print(int index, bool wHeader) const;

    static bool sortByCnr(const GnssSatellite& sat1, const GnssSatellite& sat2)
    {
        return sat1.Cnr > sat2.Cnr;
    }

public:
    std::uint8_t GnssId { 0 };
    std::uint8_t SatId { 0 };
    std::uint8_t Cnr { 0 };
    int8_t Elev { 0 };
    int16_t Azim { 0 };
    float PrRes { 0. };
    std::uint8_t Quality { 0 };
    std::uint8_t Health { 0 };
    std::uint8_t OrbitSource { 0 };
    bool Used { false };
    bool DiffCorr { false };
    bool Smoothed { false };
};

struct gpsTimestamp {
    struct timespec rising_time;
    struct timespec falling_time;
    double accuracy_ns;
    bool valid;
    int channel;
    bool rising;
    bool falling;
    int counter;
};

enum UbxDynamicModel {
    portable = 0,
    stationary = 2,
    pedestrian = 3,
    automotive = 4,
    sea = 5,
    airborne_1g = 6,
    airborne_2g = 7,
    airborne_4g = 8,
    wrist = 9,
    bike = 10
};


struct UbxDopStruct {
    uint16_t gDOP = 0, pDOP = 0, tDOP = 0, vDOP = 0, hDOP = 0, nDOP = 0, eDOP = 0;
};

inline void GnssSatellite::PrintHeader(bool wIndex)
{
    if (wIndex) {
        std::cout << "   ----------------------------------------------------------------------------------" << std::endl;
        std::cout << "   Nr   Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << std::endl;
        std::cout << "   ----------------------------------------------------------------------------------" << std::endl;
    } else {
        std::cout << "   -----------------------------------------------------------------" << std::endl;
        std::cout << "    Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << std::endl;
        std::cout << "   -----------------------------------------------------------------" << std::endl;
    }
}

inline void GnssSatellite::Print(bool wHeader) const
{
    if (wHeader) {
        std::cout << "   ------------------------------------------------------------------------------" << std::endl;
        std::cout << "    Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << std::endl;
        std::cout << "   ------------------------------------------------------------------------------" << std::endl;
    }
    std::cout << "   " << std::dec << "  " << Gnss::Id::name[static_cast<int>(GnssId)] << "   " << std::setw(3) << (int)SatId << "    ";
    std::cout << std::setw(3) << (int)Cnr << "      " << std::setw(3) << (int)Elev << "       " << std::setw(3) << (int)Azim;
    std::cout << "   " << std::setw(6) << PrRes << "    " << Quality << "   " << std::string((Used) ? "Y" : "N");
    std::cout << "    " << Health << "   " << OrbitSource << "   " << (int)Smoothed << "    " << (int)DiffCorr;
    std::cout << std::endl;
}

inline void GnssSatellite::Print(int index, bool wHeader) const
{
    if (wHeader) {
        std::cout << "   ----------------------------------------------------------------------------------" << std::endl;
        std::cout << "   Nr   Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr" << std::endl;
        std::cout << "   ----------------------------------------------------------------------------------" << std::endl;
    }
    std::cout << "   " << std::dec << std::setw(2) << index + 1 << "  " << Gnss::Id::name[static_cast<int>(GnssId)] << "   " << std::setw(3) << (int)SatId << "    ";
    std::cout << std::setw(3) << (int)Cnr << "      " << std::setw(3) << (int)Elev << "       " << std::setw(3) << (int)Azim;
    std::cout << "   " << std::setw(6) << PrRes << "    " << Quality << "   " << std::string((Used) ? "Y" : "N");
    std::cout << "    " << Health << "   " << OrbitSource << "   " << (int)Smoothed << "    " << (int)DiffCorr;
    ;
    std::cout << std::endl;
}

struct NavStatus
{
    std::uint32_t iTOW{0};
    std::uint8_t gpsFix{0};
    std::uint8_t flags{0};
    std::uint8_t flags2{0};
    std::uint32_t ttff{0};
    std::uint32_t msss{0};
};

struct NavTimeGPS
{
    std::uint32_t iTOW{0};
    std::int32_t fTOW{0};
    std::uint16_t wnR{0};
    std::int8_t leapS{0};
    std::uint8_t flags{0};

    // time accuracy estimate
    std::uint32_t tAcc{0};
};

struct NavTimeUTC
{
    std::uint32_t iTOW{0};
    std::uint32_t tAcc{0};
    std::int32_t nano{0};
    std::uint16_t year{0};
    std::uint16_t month{0};
    std::uint16_t day{0};
    std::uint16_t hour{0};
    std::uint16_t min{0};
    std::uint16_t sec{0};
    std::uint8_t flags{0};
};

struct NavClock
{
    std::uint32_t iTOW{0};
    // clock bias
    std::int32_t clkB{0};
    // clock drift
    std::int32_t clkD{0};
    std::uint32_t tAcc{0};
    std::uint32_t fAcc{0};
};

struct NavSVinfo
{
    std::uint32_t iTOW{0};
    std::uint8_t numSvs{0};
    std::uint8_t globFlags{0};
    std::size_t goodSats{0};
    std::vector<GnssSatellite> satellites{0};
};

struct NavSat
{
    std::uint32_t iTOW{0};
    std::optional<std::uint8_t> version{std::nullopt};
    std::optional<std::uint8_t> globFlags{std::nullopt};
    std::uint8_t numSvs{0};
    std::size_t goodSats{0};
    std::vector<GnssSatellite> satellites{0};
};

struct GnssPosStruct {
    std::uint32_t iTOW{0};
    std::int32_t lon{0}; // longitude 1e-7 scaling (increase by 1 means 100 nano degrees)
    std::int32_t lat{0}; // latitude 1e-7 scaling (increase by 1 means 100 nano degrees)
    std::int32_t height{0}; // height above ellipsoid
    std::int32_t hMSL{0}; // height above main sea level
    std::uint32_t hAcc{0}; // horizontal accuracy estimate
    std::uint32_t vAcc{0}; // vertical accuracy estimate
};

struct CfgAnt
{
    std::uint16_t flags{0};
    std::uint16_t pins{0};
};

struct CfgNavX5
{
    std::uint8_t version{0};
    // std::uint16_t mask1{0};
    std::uint8_t minSVs{0};
    std::uint8_t maxSVs{0};
    std::uint8_t minCNO{0};
    std::uint8_t iniFix3D{0};
    std::uint16_t wknRollover{0};
    std::uint8_t aopCfg{0};
    std::uint16_t aopOrbMaxErr{0};
};

struct CfgNav5
{
    std::uint16_t mask{0};
    std::uint8_t dynModel{0};
    std::uint8_t fixMode{0};
    std::int32_t fixedAlt{0};
    std::uint32_t fixedAltVar{0};
    std::int8_t minElev{0};
    std::uint8_t cnoThreshNumSVs{0};
    std::uint8_t cnoThresh{0};
};

struct UbxTimePulseStruct
{
    enum { ACTIVE = 0x01,
        LOCK_GPS = 0x02,
        LOCK_OTHER = 0x04,
        IS_FREQ = 0x08,
        IS_LENGTH = 0x10,
        ALIGN_TO_TOW = 0x20,
        POLARITY = 0x40,
        GRID_UTC_GPS = 0x780 };
    std::uint8_t tpIndex{0};
    std::uint8_t version{0};
    int16_t antCableDelay{0};
    int16_t rfGroupDelay{0};
    std::uint32_t freqPeriod{0};
    std::uint32_t freqPeriodLock{0};
    std::uint32_t pulseLenRatio{0};
    std::uint32_t pulseLenRatioLock{0};
    std::int32_t userConfigDelay{0};
    std::uint32_t flags{0};
};

struct CfgGNSS
{

    std::uint8_t version{0};
    std::uint8_t numTrkChHw{0};
    std::uint8_t numTrkChUse{0};
    std::uint8_t numConfigBlocks{0};
    std::vector<GnssConfigStruct> configs;
};

struct CfgMsg
{
    std::uint16_t msgID{0};
    std::uint8_t rate{0};
};

struct MonRx
{
    std::array<std::uint16_t, s_nr_targets> pending{};
    std::array<std::uint8_t, s_nr_targets> usage{};
    std::array<std::uint8_t, s_nr_targets> peakUsage{};
    std::uint8_t tUsage{0};
    std::uint8_t tPeakUsage{0};
};


struct MonTx
{
    std::array<std::uint16_t, s_nr_targets> pending{};
    std::array<std::uint8_t, s_nr_targets> usage{};
    std::array<std::uint8_t, s_nr_targets> peakUsage{};
    std::uint8_t tUsage{0};
    std::uint8_t tPeakUsage{0};
};

struct GnssMonHwStruct
{
    std::uint16_t noisePerMS{0};
    std::uint16_t agcCnt{0};
    std::uint8_t antStatus{0};
    std::uint8_t antPower{0};
    std::uint8_t flags{0};
    std::uint8_t jamInd{0};
};

struct GnssMonHw2Struct
{
    std::int8_t ofsI{0};
    std::uint8_t magI{0};
    std::int8_t ofsQ{0};
    std::uint8_t magQ{0};
    std::uint8_t cfgSrc{0};
    std::uint32_t postStatus{0};
};

struct GpsVersion
{
    std::string hwString{0};
    std::string swString{0};
    std::string prot{0};
};

struct TimTP
{

    std::uint32_t towMS{0};
    // TP time of week, sub ms
    std::uint32_t towSubMS{0};
    // quantization error
    std::int32_t qErr{0};
    // week number
    std::uint16_t week{0};
    // flags
    std::uint8_t flags{0};
    // ref info
    std::uint8_t refInfo{0};
};

struct UbxTimeMarkStruct {
    enum { TIMEBASE_LOCAL = 0x00,
        TIMEBASE_GNSS = 0x01,
        TIMEBASE_UTC = 0x02,
        TIMEBASE_OTHER = 0x03 };
    struct timespec rising = { 0, 0 };
    struct timespec falling = { 0, 0 };
    bool risingValid = false;
    bool fallingValid = false;
    std::uint32_t accuracy_ns = 0;
    bool valid = false;
    std::uint8_t timeBase = 0;
    bool utcAvailable = false;
    std::uint8_t flags = 0;
    uint16_t evtCounter = 0;
};

using UbxEvent = std::variant<
    NavStatus,
    UbxDopStruct,
    NavTimeGPS,
    NavTimeUTC,
    NavClock,
    NavSat,
    GnssPosStruct,
    CfgAnt,
    CfgNavX5,
    CfgNav5,
    UbxTimePulseStruct,
    CfgGNSS,
    CfgMsg,
    MonRx,
    MonTx,
    GnssMonHwStruct,
    GnssMonHw2Struct,
    GpsVersion,
    TimTP,
    UbxTimeMarkStruct
>;

#endif // UBLOX_STRUCTS_H
