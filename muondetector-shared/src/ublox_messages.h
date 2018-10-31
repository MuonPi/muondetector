#ifndef UBLOX_MESSAGES_H
#define UBLOX_MESSAGES_H
#include <muondetector_shared_global.h>
#define UBLOX_VERSION 7
// not in this list are all msg of types: RXM, LOG, AID and INF

// list of UBX message Cls/ID
#define MSG_ACK			0x0501
#define MSG_NAK			0x0500

#define MSG_NAV_CLOCK		0x0122
#define MSG_NAV_DGPS		0x0131
#define MSG_NAV_AOPSTATUS	0x0160
#define MSG_NAV_DOP		0x0104
#define MSG_NAV_EOE		0x0161		// not supportet on U-Blox 7
#define MSG_NAV_GEOFENCE	0x0139	// not supportet on U-Blox 7
#define MSG_NAV_ODO		0x0109		// not supportet on U-Blox 7
#define MSG_NAV_ORB		0x0134		// not supportet on U-Blox 7
#define MSG_NAV_POSECEF		0x0101
#define MSG_NAV_POSLLH		0x0102
#define MSG_NAV_PVT		0x0107
#define MSG_NAV_RESETODO	0x0110	// not supportet on U-Blox 7
#define MSG_NAV_SAT		0x0135		// not supportet on U-Blox 7
#define MSG_NAV_SBAS		0x0132
#define MSG_NAV_SOL		0x0106
#define MSG_NAV_STATUS		0x0103
#define MSG_NAV_SVINFO		0x0130
#define MSG_NAV_TIMEBDS		0x0124	// not supportet on U-Blox 7
#define MSG_NAV_TIMEGAL		0x0125	// not supportet on U-Blox 7
#define MSG_NAV_TIMEGLO		0x0123	// not supportet on U-Blox 7
#define MSG_NAV_TIMEGPS		0x0120
#define MSG_NAV_TIMELS		0x0126	// not supportet on U-Blox 7
#define MSG_NAV_TIMEUTC		0x0121
#define MSG_NAV_VELECEF		0x0111
#define MSG_NAV_VELNED		0x0112

#define MSG_CFG_ANT		0x0613
#define MSG_CFG_CFG		0x0609
#define MSG_CFG_DAT		0x0606
#define MSG_CFG_DOSC		0x0661	// not supportet on U-Blox 7 (only with time & frequency sync products)
#define MSG_CFG_DYNSEED		0x0685	// not supportet on U-Blox 7
#define MSG_CFG_ESRC		0x0660	// not supportet on U-Blox 7 (only with time & frequency sync products)
#define MSG_CFG_FIXSEED		0x0684	// not supportet on U-Blox 7
#define MSG_CFG_GEOFENCE	0x0669	// not supportet on U-Blox 7
#define MSG_CFG_GNSS		0x063e
#define MSG_CFG_INF		0x0602
#define MSG_CFG_ITFM		0x0639
#define MSG_CFG_LOGFILTER	0x0647
#define MSG_CFG_MSG		0x0601
#define MSG_CFG_NAV5		0x0624
#define MSG_CFG_NAVX5		0x0623
#define MSG_CFG_NMEA		0x0617
#define MSG_CFG_ODO		0x061e		// not supportet on U-Blox 7
#define MSG_CFG_PM2		0x063b
#define MSG_CFG_PMS		0x0686		// not supportet on U-Blox 7
#define MSG_CFG_PRT		0x0600
#define MSG_CFG_PWR		0x0657		// not supportet on U-Blox 7
#define MSG_CFG_RATE		0x0608
#define MSG_CFG_RINV		0x0634
#define MSG_CFG_RST		0x0604
#define MSG_CFG_RXM		0x0611
#define MSG_CFG_SBAS		0x0616
#define MSG_CFG_SMGR		0x0662	// not supportet on U-Blox 7 (only with time & frequency sync products)
#define MSG_CFG_TMODE2		0x063d	// not supportet on U-Blox 7 (only for timing receivers)
#define MSG_CFG_TP5		0x0631
#define MSG_CFG_TXSLOT		0x0653	// not supportet on U-Blox 7 (only with time & frequency sync products)
#define MSG_CFG_USB		0x061b

#define MSG_TIM_TP		0x0d01
#define MSG_TIM_TM2		0x0d03
#define MSG_TIM_VRFY	0x0d06

#define MSG_MON_VER		0x0a04
#define MSG_MON_GNSS		0x0a28	// not supportet on U-Blox 7
#define MSG_MON_HW		0x0a09
#define MSG_MON_HW2		0x0a0b
#define MSG_MON_IO		0x0a02
#define MSG_MON_MSGPP		0x0a06
#define MSG_MON_PATCH		0x0a27	// not supportet on U-Blox 7
#define MSG_MON_RXBUF		0x0a07
#define MSG_MON_RXR		0x0a21
#define MSG_MON_SMGR		0x0a2e	// not supportet on U-Blox 7 (only with time & frequency sync products)
#define MSG_MON_TXBUF		0x0a08

// list of NMEA message Cls/ID
#define MSG_NMEA_DTM 0xf00a
#define MSG_NMEA_GBQ 0xf044
#define MSG_NMEA_GBS 0xf009
#define MSG_NMEA_GGA 0xf000
#define MSG_NMEA_GLL 0xf001
#define MSG_NMEA_GLQ 0xf043
#define MSG_NMEA_GNQ 0xf042
#define MSG_NMEA_GNS 0xf00d
#define MSG_NMEA_GPQ 0xf040
#define MSG_NMEA_GRS 0xf006
#define MSG_NMEA_GSA 0xf002
#define MSG_NMEA_GST 0xf007
#define MSG_NMEA_GSV 0xf003
#define MSG_NMEA_RMC 0xf004
#define MSG_NMEA_TXT 0xf041
#define MSG_NMEA_VLW 0xf00f
#define MSG_NMEA_VTG 0xf005
#define MSG_NMEA_ZDA 0xf008
#define MSG_NMEA_CONFIG 0xf141
#define MSG_NMEA_POSITION 0xf100
#define MSG_NMEA_RATE 0xf140
#define MSG_NMEA_SVSTATUS 0xf103
#define MSG_NMEA_TIME 0xf104

// proto is the shortcut for protocol and
// is defined as the correct bitmask for one
// protocol
#define PROTO_UBX 1
#define PROTO_NMEA 0b10
#define PROTO_RTCM3 0b100000
#endif // UBLOX_MESSAGES_H
