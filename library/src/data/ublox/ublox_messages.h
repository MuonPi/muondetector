#ifndef UBLOX_MESSAGES_H
#define UBLOX_MESSAGES_H

#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define UBLOX_VERSION 7
// not in this list are all msg of types: LOG, AID and INF

struct UbxProtVersion {
    unsigned major;
    unsigned minor;

    bool operator<(const UbxProtVersion& other) const {
        if (major != other.major)
            return major < other.major;
        return minor < other.minor;
    }
};

enum class UBX_RESET : std::uint32_t {
    RESET_HOT = 0x00000000,
    RESET_WARM = 0x00010000,
    RESET_COLD = 0xFFFF0000,
    RESET_HW = 0x000000,
    RESET_SW = 0x00000001,
    RESET_SW_GNSS = 0x00000002,
    RESET_HW_AFTER_SHUTDOWN = 0x00000004,
    GNSS_STOP = 0x00000008,
    GNSS_START = 0x00000009
};

constexpr UBX_RESET operator|(UBX_RESET lhs, UBX_RESET rhs) {
    using T = std::underlying_type_t<UBX_RESET>;
    return static_cast<UBX_RESET>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

constexpr UBX_RESET operator&(UBX_RESET lhs, UBX_RESET rhs) {
    using T = std::underlying_type_t<UBX_RESET>;
    return static_cast<UBX_RESET>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

enum class UBX_DEV : std::uint8_t {
    DEV_BBR = 0x01,
    DEV_FLASH = 0x02,
    DEV_EEPROM = 0x04,
    DEV_SPI_FLASH = 0x10
};

constexpr UBX_DEV operator|(UBX_DEV lhs, UBX_DEV rhs) {
    using T = std::underlying_type_t<UBX_DEV>;
    return static_cast<UBX_DEV>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

constexpr UBX_DEV operator&(UBX_DEV lhs, UBX_DEV rhs) {
    using T = std::underlying_type_t<UBX_DEV>;
    return static_cast<UBX_DEV>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

constexpr UBX_DEV& operator|=(UBX_DEV& lhs, UBX_DEV rhs) {
    lhs = lhs | rhs;
    return lhs;
}

namespace UBX_MSG {
enum msg_id : std::uint16_t {
    ACK = 0x0501,
    NAK = 0x0500,

    NAV_CLOCK = 0x0122,
    NAV_DGPS = 0x0131,
    NAV_AOPSTATUS = 0x0160,
    NAV_DOP = 0x0104,
    NAV_EOE = 0x0161,      // not supportet on U-Blox 7
    NAV_GEOFENCE = 0x0139, // not supportet on U-Blox 7
    NAV_ODO = 0x0109,      // not supportet on U-Blox 7
    NAV_ORB = 0x0134,      // not supportet on U-Blox 7
    NAV_POSECEF = 0x0101,
    NAV_POSLLH = 0x0102,
    NAV_PVT = 0x0107,
    NAV_RESETODO = 0x0110, // not supportet on U-Blox 7
    NAV_SAT = 0x0135,      // not supportet on U-Blox 7
    NAV_SBAS = 0x0132,
    NAV_SOL = 0x0106,
    NAV_STATUS = 0x0103,
    NAV_SVINFO = 0x0130,
    NAV_TIMEBDS = 0x0124, // not supportet on U-Blox 7
    NAV_TIMEGAL = 0x0125, // not supportet on U-Blox 7
    NAV_TIMEGLO = 0x0123, // not supportet on U-Blox 7
    NAV_TIMEGPS = 0x0120,
    NAV_TIMELS = 0x0126, // not supportet on U-Blox 7
    NAV_TIMEUTC = 0x0121,
    NAV_VELECEF = 0x0111,
    NAV_VELNED = 0x0112,

    CFG_ANT = 0x0613,
    CFG_CFG = 0x0609,
    CFG_DAT = 0x0606,
    CFG_DOSC = 0x0661,     // not supportet on U-Blox 7 (only with time & frequency sync products)
    CFG_DYNSEED = 0x0685,  // not supportet on U-Blox 7
    CFG_ESRC = 0x0660,     // not supportet on U-Blox 7 (only with time & frequency sync products)
    CFG_FIXSEED = 0x0684,  // not supportet on U-Blox 7
    CFG_GEOFENCE = 0x0669, // not supportet on U-Blox 7
    CFG_GNSS = 0x063e,
    CFG_INF = 0x0602,
    CFG_ITFM = 0x0639,
    CFG_LOGFILTER = 0x0647,
    CFG_MSG = 0x0601,
    CFG_NAV5 = 0x0624,
    CFG_NAVX5 = 0x0623,
    CFG_NMEA = 0x0617,
    CFG_ODO = 0x061e, // not supportet on U-Blox 7
    CFG_PM2 = 0x063b,
    CFG_PMS = 0x0686, // not supportet on U-Blox 7
    CFG_PRT = 0x0600,
    CFG_PWR = 0x0657, // not supportet on U-Blox 7
    CFG_RATE = 0x0608,
    CFG_RINV = 0x0634,
    CFG_RST = 0x0604,
    CFG_RXM = 0x0611,
    CFG_SBAS = 0x0616,
    CFG_SMGR = 0x0662,   // not supportet on U-Blox 7 (only with time & frequency sync products)
    CFG_TMODE2 = 0x063d, // not supportet on U-Blox 7 (only for timing receivers)
    CFG_TP5 = 0x0631,
    CFG_TXSLOT = 0x0653, // not supportet on U-Blox 7 (only with time & frequency sync products)
    CFG_USB = 0x061b,

    TIM_TP = 0x0d01,
    TIM_TM2 = 0x0d03,
    TIM_VRFY = 0x0d06,

    MON_VER = 0x0a04,
    MON_GNSS = 0x0a28, // not supportet on U-Blox 7
    MON_HW = 0x0a09,
    MON_HW2 = 0x0a0b,
    MON_IO = 0x0a02,
    MON_MSGPP = 0x0a06,
    MON_PATCH = 0x0a27, // not supportet on U-Blox 7
    MON_RXBUF = 0x0a07,
    MON_RXR = 0x0a21,
    MON_SMGR = 0x0a2e, // not supportet on U-Blox 7 (only with time & frequency sync products)
    MON_TXBUF = 0x0a08,

    // MEA message Cls/ID
    NMEA_DTM = 0xf00a,
    NMEA_GBQ = 0xf044,
    NMEA_GBS = 0xf009,
    NMEA_GGA = 0xf000,
    NMEA_GLL = 0xf001,
    NMEA_GLQ = 0xf043,
    NMEA_GNQ = 0xf042,
    NMEA_GNS = 0xf00d,
    NMEA_GPQ = 0xf040,
    NMEA_GRS = 0xf006,
    NMEA_GSA = 0xf002,
    NMEA_GST = 0xf007,
    NMEA_GSV = 0xf003,
    NMEA_RMC = 0xf004,
    NMEA_TXT = 0xf041,
    NMEA_VLW = 0xf00f,
    NMEA_VTG = 0xf005,
    NMEA_ZDA = 0xf008,
    NMEA_CONFIG = 0xf141,
    NMEA_POSITION = 0xf100,
    NMEA_RATE = 0xf140,
    NMEA_SVSTATUS = 0xf103,
    NMEA_TIME = 0xf104,

    RXM_RAW = 0x0210,   /* ubx message id: raw measurement data */
    RXM_SFRB = 0x0211,  /* ubx message id: subframe buffer */
    RXM_SFRBX = 0x0213, /* ubx message id: raw subframe data */
    RXM_RAWX = 0x0215,  /* ubx message id: multi-gnss raw meas data */

    TRK_D5 = 0x030A,   /* ubx message id: trace mesurement data */
    TRK_MEAS = 0x0310, /* ubx message id: trace mesurement data */
    TRK_SFRBX = 0x030F /* ubx message id: trace subframe buffer */
};

const static std::map<msg_id, std::string> msg_string{{ACK, "ACK-ACK"},
                                                      {NAK, "ACK-NAK"},

                                                      {NAV_CLOCK, "NAV-CLOCK"},
                                                      {NAV_DGPS, "NAV-DGPS"},
                                                      {NAV_AOPSTATUS, "NAV-AOPSTATUS"},
                                                      {NAV_DOP, "NAV-DOP"},
                                                      {NAV_POSECEF, "NAV-POSECEF"},
                                                      {NAV_POSLLH, "NAV-POSLLH"},
                                                      {NAV_PVT, "NAV-PVT"},
                                                      {NAV_SBAS, "NAV-SBAS"},
                                                      {NAV_SOL, "NAV-SOL"},
                                                      {NAV_STATUS, "NAV-STATUS"},
                                                      {NAV_SVINFO, "NAV-SVINFO"},
                                                      {NAV_TIMEGPS, "NAV-TIMEGPS"},
                                                      {NAV_TIMEUTC, "NAV-TIMEUTC"},
                                                      {NAV_VELECEF, "NAV-VELECEF"},
                                                      {NAV_VELNED, "NAV-VELNED"},
                                                      // CFG
                                                      {CFG_ANT, "CFG-ANT"},
                                                      {CFG_CFG, "CFG-CFG"},
                                                      {CFG_DAT, "CFG-DAT"},
                                                      {CFG_GNSS, "CFG-GNSS"},
                                                      {CFG_INF, "CFG-INF"},
                                                      {CFG_ITFM, "CFG-ITFM"},
                                                      {CFG_LOGFILTER, "CFG-LOGFILTER"},
                                                      {CFG_MSG, "CFG-MSG"},
                                                      {CFG_NAV5, "CFG-NAV5"},
                                                      {CFG_NAVX5, "CFG-NAV5X"},
                                                      {CFG_NMEA, "CFG-NMEA"},
                                                      {CFG_PM2, "CFG-PM2"},
                                                      {CFG_PRT, "CFG-PRT"},
                                                      {CFG_RATE, "CFG-RATE"},
                                                      {CFG_RINV, "CFG-RINV"},
                                                      {CFG_RST, "CFG-RST"},
                                                      {CFG_RXM, "CFG-RXM"},
                                                      {CFG_SBAS, "CFG-SBAS"},
                                                      {CFG_TP5, "CFG-TP5"},
                                                      {CFG_USB, "CFG-USB"},
                                                      // TIM
                                                      {TIM_TP, "TIM-TP"},
                                                      {TIM_TM2, "TIM-TM2"},
                                                      {TIM_VRFY, "TIM-VRFY"},
                                                      // MON
                                                      {MON_VER, "MON-VER"},
                                                      {MON_HW, "MON-HW"},
                                                      {MON_HW2, "MON-HW2"},
                                                      {MON_IO, "MON-IO"},
                                                      {MON_MSGPP, "MON-MSGP"},
                                                      {MON_RXBUF, "MON-RXBUF"},
                                                      {MON_RXR, "MON-RXR"},
                                                      {MON_TXBUF, "MON-TXBUF"},

                                                      // the messages only used in Ublox-8
                                                      {NAV_EOE, "NAV-EOE"},
                                                      {NAV_GEOFENCE, "NAV-GEOFENCE"},
                                                      {NAV_ODO, "NAV-ODO"},
                                                      {NAV_ORB, "NAV-ORB"},
                                                      {NAV_RESETODO, "NAV-RESETODO"},
                                                      {NAV_SAT, "NAV-SAT"},
                                                      {NAV_TIMEBDS, "NAV-TIMEBDS"},
                                                      {NAV_TIMEGAL, "NAV-TIMEGAL"},
                                                      {NAV_TIMEGLO, "NAV-TIMEGLO"},
                                                      {NAV_TIMELS, "NAV-TIMELS"},

                                                      // NMEA
                                                      {NMEA_DTM, "NMEA-DTM"},
                                                      {NMEA_GBQ, "NMEA-GBQ"},
                                                      {NMEA_GBS, "NMEA-GBS"},
                                                      {NMEA_GGA, "NMEA-GGA"},
                                                      {NMEA_GLL, "NMEA-GLL"},
                                                      {NMEA_GLQ, "NMEA-GLQ"},
                                                      {NMEA_GNQ, "NMEA-GNQ"},
                                                      {NMEA_GNS, "NMEA-GNS"},
                                                      {NMEA_GPQ, "NMEA-GPQ"},
                                                      {NMEA_GRS, "NMEA-GRS"},
                                                      {NMEA_GSA, "NMEA-GSA"},
                                                      {NMEA_GST, "NMEA-GST"},
                                                      {NMEA_GSV, "NMEA-GSV"},
                                                      {NMEA_RMC, "NMEA-RMC"},
                                                      {NMEA_TXT, "NMEA-TXT"},
                                                      {NMEA_VLW, "NMEA-VLW"},
                                                      {NMEA_VTG, "NMEA-VTG"},
                                                      {NMEA_ZDA, "NMEA-ZDA"},
                                                      {NMEA_CONFIG, "NMEA-CONFIG"},
                                                      {NMEA_POSITION, "NMEA-POSITION"},
                                                      {NMEA_RATE, "NMEA-RATE"},
                                                      {NMEA_SVSTATUS, "NMEA-SVSTATUS"},
                                                      {NMEA_TIME, "NMEA-TIME"},

                                                      {RXM_RAW, "RXM-RAW"},
                                                      {RXM_SFRB, "RXMSFRB"},
                                                      {RXM_RAWX, "RXM-RAWX"},
                                                      {TRK_D5, "TRK-D5"},
                                                      {TRK_MEAS, "TRK-MEAS"},
                                                      {TRK_SFRBX, "TRK-SFRBX"}};
} // namespace UBX_MSG

// All ids of rate configurable messages which we want to configure
inline const std::vector<UBX_MSG::msg_id> rateCfgMsgID{
    {UBX_MSG::TIM_TM2,       UBX_MSG::TIM_TP,      UBX_MSG::NAV_CLOCK,   UBX_MSG::NAV_DGPS,
     UBX_MSG::NAV_AOPSTATUS, UBX_MSG::NAV_DOP,     UBX_MSG::NAV_POSECEF, UBX_MSG::NAV_POSLLH,
     UBX_MSG::NAV_PVT,       UBX_MSG::NAV_SBAS,    UBX_MSG::NAV_SOL,     UBX_MSG::NAV_STATUS,
     UBX_MSG::NAV_SVINFO,    UBX_MSG::NAV_TIMEGPS, UBX_MSG::NAV_TIMEUTC, UBX_MSG::NAV_VELECEF,
     UBX_MSG::NAV_VELNED,    UBX_MSG::MON_HW,      UBX_MSG::MON_HW2,     UBX_MSG::MON_IO,
     UBX_MSG::MON_MSGPP,     UBX_MSG::MON_RXBUF,   UBX_MSG::MON_RXR,     UBX_MSG::MON_TXBUF}};

// All CMD commands expect ack, others dont
inline const std::unordered_set<UBX_MSG::msg_id> idsExpectedAck{
    UBX_MSG::CFG_ANT,     UBX_MSG::CFG_CFG,    UBX_MSG::CFG_DAT,     UBX_MSG::CFG_DOSC,
    UBX_MSG::CFG_DYNSEED, UBX_MSG::CFG_ESRC,   UBX_MSG::CFG_FIXSEED, UBX_MSG::CFG_GEOFENCE,
    UBX_MSG::CFG_GNSS,    UBX_MSG::CFG_INF,    UBX_MSG::CFG_ITFM,    UBX_MSG::CFG_LOGFILTER,
    UBX_MSG::CFG_MSG,     UBX_MSG::CFG_NAV5,   UBX_MSG::CFG_NAVX5,   UBX_MSG::CFG_NMEA,
    UBX_MSG::CFG_ODO,     UBX_MSG::CFG_PM2,    UBX_MSG::CFG_PMS,     UBX_MSG::CFG_PRT,
    UBX_MSG::CFG_PWR,     UBX_MSG::CFG_RATE,   UBX_MSG::CFG_RINV,    UBX_MSG::CFG_RST,
    UBX_MSG::CFG_RXM,     UBX_MSG::CFG_SBAS,   UBX_MSG::CFG_SMGR,    UBX_MSG::CFG_TMODE2,
    UBX_MSG::CFG_TP5,     UBX_MSG::CFG_TXSLOT, UBX_MSG::CFG_USB};

// Rates as they will be set at initialization
inline const std::unordered_map<UBX_MSG::msg_id, std::pair<std::uint8_t, std::uint8_t>>
    defaultRates{
        {UBX_MSG::TIM_TM2, {1, 1}},       {UBX_MSG::TIM_TP, {1, 0}},
        {UBX_MSG::NAV_TIMEUTC, {1, 131}}, {UBX_MSG::MON_HW, {1, 47}},
        {UBX_MSG::MON_HW2, {1, 49}},      {UBX_MSG::NAV_POSLLH, {1, 43}},
        {UBX_MSG::NAV_TIMEGPS, {1, 0}},   {UBX_MSG::NAV_STATUS, {1, 71}},
        {UBX_MSG::NAV_CLOCK, {1, 189}},   {UBX_MSG::MON_RXBUF, {1, 53}},
        {UBX_MSG::MON_TXBUF, {1, 51}},    {UBX_MSG::NAV_SBAS, {1, 0}},
        {UBX_MSG::NAV_DOP, {1, 254}},
    };
// proto is the shortcut for protocol and
// is defined as the correct bitmask for one
// protocol
#define PROTO_UBX 1
#define PROTO_NMEA 0b10
#define PROTO_RTCM3 0b100000
#endif // UBLOX_MESSAGES_H
