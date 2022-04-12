#ifndef UBLOX_MESSAGES_H
#define UBLOX_MESSAGES_H

#include "muondetector_shared_global.h"
#include <map>
#include <string>

#define UBLOX_VERSION 7
// not in this list are all msg of types: LOG, AID and INF

namespace UBX_MSG {
enum msg_id: std::uint16_t {
    ACK = 0x0501,
    NAK = 0x0500,
    
    NAV_CLOCK = 0x0122,
    NAV_DGPS = 0x0131,
    NAV_AOPSTATUS = 0x0160,
    NAV_DOP = 0x0104,
    NAV_EOE = 0x0161, // not supportet on U-Blox 7
    NAV_GEOFENCE = 0x0139, // not supportet on U-Blox 7
    NAV_ODO = 0x0109, // not supportet on U-Blox 7
    NAV_ORB = 0x0134, // not supportet on U-Blox 7
    NAV_POSECEF = 0x0101,
    NAV_POSLLH = 0x0102,
    NAV_PVT = 0x0107,
    NAV_RESETODO = 0x0110, // not supportet on U-Blox 7
    NAV_SAT = 0x0135, // not supportet on U-Blox 7
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
    CFG_DOSC = 0x0661, // not supportet on U-Blox 7 (only with time & frequency sync products)
    CFG_DYNSEED = 0x0685, // not supportet on U-Blox 7
    CFG_ESRC = 0x0660, // not supportet on U-Blox 7 (only with time & frequency sync products)
    CFG_FIXSEED = 0x0684, // not supportet on U-Blox 7
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
    CFG_SMGR = 0x0662, // not supportet on U-Blox 7 (only with time & frequency sync products)
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

    //MEA message Cls/ID
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
    
    RXM_RAW = 0x0210,      /* ubx message id: raw measurement data */
    RXM_SFRB = 0x0211,      /* ubx message id: subframe buffer */
    RXM_SFRBX = 0x0213,      /* ubx message id: raw subframe data */
    RXM_RAWX = 0x0215,      /* ubx message id: multi-gnss raw meas data */

    TRK_D5 = 0x030A,      /* ubx message id: trace mesurement data */
    TRK_MEAS = 0x0310,      /* ubx message id: trace mesurement data */
    TRK_SFRBX = 0x030F      /* ubx message id: trace subframe buffer */
};

const static std::map<msg_id, std::string> msg_string {
    { ACK, "ACK-ACK" },
    { NAK, "ACK-NAK" },
    
    { NAV_CLOCK, "NAV-CLOCK" },
    { NAV_DGPS, "NAV-DGPS" },
    { NAV_AOPSTATUS, "NAV-AOPSTATUS" },
    { NAV_DOP, "NAV-DOP" },
    { NAV_POSECEF, "NAV-POSECEF" },
    { NAV_POSLLH, "NAV-POSLLH" },
    { NAV_PVT, "NAV-PVT" },
    { NAV_SBAS, "NAV-SBAS" },
    { NAV_SOL, "NAV-SOL" },
    { NAV_STATUS, "NAV-STATUS" },
    { NAV_SVINFO, "NAV-SVINFO" },
    { NAV_TIMEGPS, "NAV-TIMEGPS" },
    { NAV_TIMEUTC, "NAV-TIMEUTC" },
    { NAV_VELECEF, "NAV-VELECEF" },
    { NAV_VELNED, "NAV-VELNED" },
    // CFG
    { CFG_ANT, "CFG-ANT" },
    { CFG_CFG, "CFG-CFG" },
    { CFG_DAT, "CFG-DAT" },
    { CFG_GNSS, "CFG-GNSS" },
    { CFG_INF, "CFG-INF" },
    { CFG_ITFM, "CFG-ITFM" },
    { CFG_LOGFILTER, "CFG-LOGFILTER" },
    { CFG_MSG, "CFG-MSG" },
    { CFG_NAV5, "CFG-NAV5" },
    { CFG_NAVX5, "CFG-NAV5X" },
    { CFG_NMEA, "CFG-NMEA" },
    { CFG_PM2, "CFG-PM2" },
    { CFG_PRT, "CFG-PRT" },
    { CFG_RATE, "CFG-RATE" },
    { CFG_RINV, "CFG-RINV" },
    { CFG_RST, "CFG-RST" },
    { CFG_RXM, "CFG-RXM" },
    { CFG_SBAS, "CFG-SBAS" },
    { CFG_TP5, "CFG-TP5" },
    { CFG_USB, "CFG-USB" },
    // TIM
    { TIM_TP, "TIM-TP" },
    { TIM_TM2, "TIM-TM2" },
    { TIM_VRFY, "TIM-VRFY" },
    // MON
    { MON_VER, "MON-VER" },
    { MON_HW, "MON-HW" },
    { MON_HW2, "MON-HW2" },
    { MON_IO, "MON-IO" },
    { MON_MSGPP, "MON-MSGP" },
    { MON_RXBUF, "MON-RXBUF" },
    { MON_RXR, "MON-RXR" },
    { MON_TXBUF, "MON-TXBUF" },

    // the messages only used in Ublox-8
    { NAV_EOE, "NAV-EOE" },
    { NAV_GEOFENCE, "NAV-GEOFENCE" },
    { NAV_ODO, "NAV-ODO" },
    { NAV_ORB, "NAV-ORB" },
    { NAV_RESETODO, "NAV-RESETODO" },
    { NAV_SAT, "NAV-SAT" },
    { NAV_TIMEBDS, "NAV-TIMEBDS" },
    { NAV_TIMEGAL, "NAV-TIMEGAL" },
    { NAV_TIMEGLO, "NAV-TIMEGLO" },
    { NAV_TIMELS, "NAV-TIMELS" },

    // NMEA
    { NMEA_DTM, "NMEA-DTM" },
    { NMEA_GBQ, "NMEA-GBQ" },
    { NMEA_GBS, "NMEA-GBS" },
    { NMEA_GGA, "NMEA-GGA" },
    { NMEA_GLL, "NMEA-GLL" },
    { NMEA_GLQ, "NMEA-GLQ" },
    { NMEA_GNQ, "NMEA-GNQ" },
    { NMEA_GNS, "NMEA-GNS" },
    { NMEA_GPQ, "NMEA-GPQ" },
    { NMEA_GRS, "NMEA-GRS" },
    { NMEA_GSA, "NMEA-GSA" },
    { NMEA_GST, "NMEA-GST" },
    { NMEA_GSV, "NMEA-GSV" },
    { NMEA_RMC, "NMEA-RMC" },
    { NMEA_TXT, "NMEA-TXT" },
    { NMEA_VLW, "NMEA-VLW" },
    { NMEA_VTG, "NMEA-VTG" },
    { NMEA_ZDA, "NMEA-ZDA" },
    { NMEA_CONFIG, "NMEA-CONFIG" },
    { NMEA_POSITION, "NMEA-POSITION" },
    { NMEA_RATE, "NMEA-RATE" },
    { NMEA_SVSTATUS, "NMEA-SVSTATUS" },
    { NMEA_TIME, "NMEA-TIME" },

    { RXM_RAW, "RXM-RAW" },
    { RXM_SFRB, "RXMSFRB" },
    { RXM_RAWX, "RXM-RAWX" },
    { TRK_D5, "TRK-D5" },
    { TRK_MEAS, "TRK-MEAS" },
    { TRK_SFRBX, "TRK-SFRBX" }
};
}

// proto is the shortcut for protocol and
// is defined as the correct bitmask for one
// protocol
#define PROTO_UBX 1
#define PROTO_NMEA 0b10
#define PROTO_RTCM3 0b100000
#endif // UBLOX_MESSAGES_H
