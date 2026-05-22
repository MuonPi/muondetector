#ifndef UBX_EVENT_H
#define UBX_EVENT_H

#include "data/ublox/ublox_messages.h"
#include "data/ublox/ublox_structs.h"

#include <sstream>

template <typename T>
struct MsgId;

class GnssSatellite {
  public:
    GnssSatellite() = default;
    GnssSatellite(int gnssId, int satId, int cnr, int elev, int azim, float prRes,
                  std::uint32_t flags)
        : GnssId(gnssId), SatId(satId), Cnr(cnr), Elev(elev), Azim(azim), PrRes(prRes) {
        Quality = (int) (flags & 0x07);
        if (flags & 0x08)
            Used = true;
        else
            Used = false;
        Health = (int) (flags >> 4 & 0x03);
        OrbitSource = (flags >> 8 & 0x07);
        Smoothed = (flags & 0x80);
        DiffCorr = (flags & 0x40);
    }

    GnssSatellite(int gnssId, int satId, int cnr, int elev, int azim, float prRes, int quality,
                  int health, int orbitSource, bool used, bool diffCorr, bool smoothed)
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
        , Smoothed(smoothed) {}

    ~GnssSatellite() {}

    static void PrintHeader(std::stringstream& sstr, bool wIndex);
    void Print(std::stringstream& sstr, bool wHeader) const;
    void Print(std::stringstream& sstr, int index, bool wHeader) const;

    static bool sortByCnr(const GnssSatellite& sat1, const GnssSatellite& sat2) {
        return sat1.Cnr > sat2.Cnr;
    }

  public:
    std::uint8_t GnssId{0};
    std::uint8_t SatId{0};
    std::uint8_t Cnr{0};
    int8_t Elev{0};
    int16_t Azim{0};
    float PrRes{0.};
    std::uint8_t Quality{0};
    std::uint8_t Health{0};
    std::uint8_t OrbitSource{0};
    bool Used{false};
    bool DiffCorr{false};
    bool Smoothed{false};
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

inline void GnssSatellite::PrintHeader(std::stringstream& sstr, bool wIndex) {
    if (wIndex) {
        sstr << "   "
                "----------------------------------------------------------------------------------"
             << std::endl;
        sstr << "   Nr   Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth "
                "DiffCorr"
             << std::endl;
        sstr << "   "
                "----------------------------------------------------------------------------------"
             << std::endl;
    } else {
        sstr << "   -----------------------------------------------------------------" << std::endl;
        sstr << "    Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr"
             << std::endl;
        sstr << "   -----------------------------------------------------------------" << std::endl;
    }
}

inline void GnssSatellite::Print(std::stringstream& sstr, bool wHeader) const {
    if (wHeader) {
        sstr << "   ------------------------------------------------------------------------------"
             << std::endl;
        sstr << "    Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr"
             << std::endl;
        sstr << "   ------------------------------------------------------------------------------"
             << std::endl;
    }
    sstr << "   " << std::dec << "  " << Gnss::Id::name[static_cast<int>(GnssId)] << "   "
         << std::setw(3) << (int) SatId << "    ";
    sstr << std::setw(3) << (int) Cnr << "      " << std::setw(3) << (int) Elev << "       "
         << std::setw(3) << (int) Azim;
    sstr << "   " << std::setw(6) << PrRes << "    " << Quality << "   "
         << std::string((Used) ? "Y" : "N");
    sstr << "    " << Health << "   " << OrbitSource << "   " << (int) Smoothed << "    "
         << (int) DiffCorr;
    sstr << std::endl;
}

inline void GnssSatellite::Print(std::stringstream& sstr, int index, bool wHeader) const {
    if (wHeader) {
        sstr << "   "
                "----------------------------------------------------------------------------------"
             << std::endl;
        sstr << "   Nr   Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth "
                "DiffCorr"
             << std::endl;
        sstr << "   "
                "----------------------------------------------------------------------------------"
             << std::endl;
    }
    sstr << "   " << std::dec << std::setw(2) << index + 1 << "  "
         << Gnss::Id::name[static_cast<int>(GnssId)] << "   " << std::setw(3) << (int) SatId
         << "    ";
    sstr << std::setw(3) << (int) Cnr << "      " << std::setw(3) << (int) Elev << "       "
         << std::setw(3) << (int) Azim;
    sstr << "   " << std::setw(6) << PrRes << "    " << Quality << "   "
         << std::string((Used) ? "Y" : "N");
    sstr << "    " << Health << "   " << OrbitSource << "   " << (int) Smoothed << "    "
         << (int) DiffCorr;
    ;
    sstr << std::endl;
}

struct UbxDopStruct {
    std::uint16_t gDOP = 0, pDOP = 0, tDOP = 0, vDOP = 0, hDOP = 0, nDOP = 0, eDOP = 0;
};
template <>
struct MsgId<UbxDopStruct> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::NAV_DOP;
};

struct NavStatus {
    std::uint32_t iTOW{0};
    std::uint8_t gpsFix{0};
    std::uint8_t flags{0};
    std::uint8_t flags2{0};
    std::uint32_t ttff{0};
    std::uint32_t msss{0};
};
template <>
struct MsgId<NavStatus> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::NAV_STATUS;
};

struct NavTimeGPS {
    std::uint32_t iTOW{0};
    std::int32_t fTOW{0};
    std::uint16_t wnR{0};
    std::int8_t leapS{0};
    std::uint8_t flags{0};

    // time accuracy estimate
    std::uint32_t tAcc{0};
};

template <>
struct MsgId<NavTimeGPS> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::NAV_TIMEGPS;
};

struct NavTimeUTC {
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
template <>
struct MsgId<NavTimeUTC> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::NAV_TIMEUTC;
};

struct NavClock {
    std::uint32_t iTOW{0};
    // clock bias
    std::int32_t clkB{0};
    // clock drift
    std::int32_t clkD{0};
    std::uint32_t tAcc{0};
    std::uint32_t fAcc{0};
};
template <>
struct MsgId<NavClock> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::NAV_CLOCK;
};

struct NavSVinfo {
    std::uint32_t iTOW{0};
    std::uint8_t numSvs{0};
    std::uint8_t globFlags{0};
    std::size_t goodSats{0};
    std::vector<GnssSatellite> satellites{0};
};
template <>
struct MsgId<NavSVinfo> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::NAV_SVINFO;
};

struct NavSat {
    std::uint32_t iTOW{0};
    std::optional<std::uint8_t> version{std::nullopt};
    std::optional<std::uint8_t> globFlags{std::nullopt};
    std::uint8_t numSvs{0};
    std::size_t goodSats{0};
    std::vector<GnssSatellite> satellites{};
};
template <>
struct MsgId<NavSat> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::NAV_SAT;
};

struct GnssPosStruct {
    std::uint32_t iTOW{0};
    std::int32_t lon{0};    // longitude 1e-7 scaling (increase by 1 means 100 nano degrees)
    std::int32_t lat{0};    // latitude 1e-7 scaling (increase by 1 means 100 nano degrees)
    std::int32_t height{0}; // height above ellipsoid
    std::int32_t hMSL{0};   // height above main sea level
    std::uint32_t hAcc{0};  // horizontal accuracy estimate
    std::uint32_t vAcc{0};  // vertical accuracy estimate
};
template <>
struct MsgId<GnssPosStruct> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::NAV_POSLLH;
};

struct CfgAnt {
    std::uint16_t flags{0};
    std::uint16_t pins{0};
};
template <>
struct MsgId<CfgAnt> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::CFG_ANT;
};

struct CfgNavX5 {
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
template <>
struct MsgId<CfgNavX5> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::CFG_NAVX5;
};

struct CfgNav5 {
    std::uint16_t mask{0};
    std::uint8_t dynModel{0};
    std::uint8_t fixMode{0};
    std::int32_t fixedAlt{0};
    std::uint32_t fixedAltVar{0};
    std::int8_t minElev{0};
    std::uint8_t cnoThreshNumSVs{0};
    std::uint8_t cnoThresh{0};
};
template <>
struct MsgId<CfgNav5> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::CFG_NAV5;
};

struct UbxTimePulseStruct {
    enum {
        ACTIVE = 0x01,
        LOCK_GPS = 0x02,
        LOCK_OTHER = 0x04,
        IS_FREQ = 0x08,
        IS_LENGTH = 0x10,
        ALIGN_TO_TOW = 0x20,
        POLARITY = 0x40,
        GRID_UTC_GPS = 0x780
    };
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
template <>
struct MsgId<UbxTimePulseStruct> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::CFG_TP5;
};

struct CfgGNSS {
    std::uint8_t version{0};
    std::uint8_t numTrkChHw{0};
    std::uint8_t numTrkChUse{0};
    std::uint8_t numConfigBlocks{0};
    std::vector<GnssConfigStruct> configs;
};
template <>
struct MsgId<CfgGNSS> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::CFG_GNSS;
};

struct CfgMsg {
    std::uint16_t msgID{0};
    int rate{0};
};
template <>
struct MsgId<CfgMsg> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::CFG_MSG;
};

struct MonRx {
    std::array<std::uint16_t, s_nr_targets> pending{};
    std::array<std::uint8_t, s_nr_targets> usage{};
    std::array<std::uint8_t, s_nr_targets> peakUsage{};
    std::uint8_t tUsage{0};
    std::uint8_t tPeakUsage{0};
};
template <>
struct MsgId<MonRx> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::MON_RXBUF;
};

struct MonTx {
    std::array<std::uint16_t, s_nr_targets> pending{};
    std::array<std::uint8_t, s_nr_targets> usage{};
    std::array<std::uint8_t, s_nr_targets> peakUsage{};
    std::uint8_t tUsage{0};
    std::uint8_t tPeakUsage{0};
};
template <>
struct MsgId<MonTx> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::MON_TXBUF;
};

struct GnssMonHwStruct {
    std::uint16_t noisePerMS{0};
    std::uint16_t agcCnt{0};
    std::uint8_t antStatus{0};
    std::uint8_t antPower{0};
    std::uint8_t flags{0};
    std::uint8_t jamInd{0};
};
template <>
struct MsgId<GnssMonHwStruct> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::MON_HW;
};

struct GnssMonHw2Struct {
    std::int8_t ofsI{0};
    std::uint8_t magI{0};
    std::int8_t ofsQ{0};
    std::uint8_t magQ{0};
    std::uint8_t cfgSrc{0};
    std::uint32_t postStatus{0};
};
template <>
struct MsgId<GnssMonHw2Struct> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::MON_HW2;
};

struct GpsVersion {
    std::string hwString{};
    std::string swString{};
    std::string prot{};
};
template <>
struct MsgId<GpsVersion> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::MON_VER;
};

struct TimTP {

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
template <>
struct MsgId<TimTP> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::TIM_TP;
};

struct UbxTimeMarkStruct {
    enum {
        TIMEBASE_LOCAL = 0x00,
        TIMEBASE_GNSS = 0x01,
        TIMEBASE_UTC = 0x02,
        TIMEBASE_OTHER = 0x03
    };
    struct timespec rising = {0, 0};
    struct timespec falling = {0, 0};
    bool risingValid = false;
    bool fallingValid = false;
    std::uint32_t accuracy_ns = 0;
    bool valid = false;
    std::uint8_t timeBase = 0;
    bool utcAvailable = false;
    std::uint8_t flags = 0;
    std::uint16_t evtCounter = 0;
};
template <>
struct MsgId<UbxTimeMarkStruct> {
    static constexpr UBX_MSG::msg_id value = UBX_MSG::TIM_TM2;
};

struct UbxMsgRates {
    std::vector<CfgMsg> data;
};

struct UbxAckNak {
    std::uint16_t msgID{0};
    std::uint16_t payload{0};
};

struct UbxAckAck {
    std::uint16_t msgID{0};
};

using UbxEvent = std::variant<NavStatus, UbxDopStruct, NavTimeGPS, NavTimeUTC, NavClock, NavSat,
                              GnssPosStruct, CfgAnt, CfgNavX5, CfgNav5, UbxTimePulseStruct, CfgGNSS,
                              CfgMsg, MonRx, MonTx, GnssMonHwStruct, GnssMonHw2Struct, GpsVersion,
                              TimTP, UbxTimeMarkStruct, UbxAckNak, UbxAckAck>;

#endif // UBX_EVENT_H