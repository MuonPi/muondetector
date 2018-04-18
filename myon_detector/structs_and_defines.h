#ifndef STRUCTS_AND_DEFINES_H
#define STRUCTS_AND_DEFINES__H

#define MSG_ACK			0x0501
#define MSG_NAK			0x0500

#define MSG_NAV_CLOCK		0x0122
#define MSG_NAV_DGPS		0x0131
#define MSG_NAV_AOPSTATUS	0x0160
#define MSG_NAV_DOP		0x0104
#define MSG_NAV_EOE		0x0161
#define MSG_NAV_GEOFENCE	0x0139
#define MSG_NAV_ODO		0x0109
#define MSG_NAV_ORB		0x0134
#define MSG_NAV_POSECEF		0x0101
#define MSG_NAV_LLH		0x0102
#define MSG_NAV_PVT		0x0107
#define MSG_NAV_RESETODO	0x0110
#define MSG_NAV_SAT		0x0135
#define MSG_NAV_SBAS		0x0132
#define MSG_NAV_SOL		0x0106
#define MSG_NAV_STATUS		0x0103
#define MSG_NAV_SVINFO		0x0130
#define MSG_NAV_TIMEBDS		0x0124
#define MSG_NAV_TIMEGAL		0x0125
#define MSG_NAV_TIMEGLO		0x0123
#define MSG_NAV_TIMEGPS		0x0120
#define MSG_NAV_TIMELS		0x0126
#define MSG_NAV_TIMEUTC		0x0121
#define MSG_NAV_VELECEF		0x0111
#define MSG_NAV_VELNED		0x0112

#define MSG_CFG_ANT		0x0613
#define MSG_CFG_CFG		0x0609
#define MSG_CFG_DAT		0x0606
#define MSG_CFG_DOSC		0x0661
#define MSG_CFG_DYNSEED		0x0685
#define MSG_CFG_ESRC		0x0660
#define MSG_CFG_FIXSEED		0x0684
#define MSG_CFG_GEOFENCE	0x0669
#define MSG_CFG_GNSS		0x063e
#define MSG_CFG_INF		0x0602
#define MSG_CFG_ITFM		0x0639
#define MSG_CFG_LOGFILTER	0x0647
#define MSG_CFG_MSG		0x0601
#define MSG_CFG_NAV5		0x0624
#define MSG_CFG_NAVX5		0x0623
#define MSG_CFG_NMEA		0x0617
#define MSG_CFG_ODO		0x061e
#define MSG_CFG_PM2		0x063b
#define MSG_CFG_PMS		0x0686
#define MSG_CFG_PRT		0x0600
#define MSG_CFG_PWR		0x0657
#define MSG_CFG_RATE		0x0608
#define MSG_CFG_RINV		0x0634
#define MSG_CFG_RST		0x0604
#define MSG_CFG_RXM		0x0611
#define MSG_CFG_SBAS		0x0616
#define MSG_CFG_SMGR		0x0662
#define MSG_CFG_TMODE2		0x063d
#define MSG_CFG_TP5		0x0631
#define MSG_CFG_TXSLOT		0x0653
#define MSG_CFG_USB		0x061b

#define MSG_TIM_TP		0x0d01
#define MSG_TIM_TM2		0x0d03

#define MSG_MON_VER		0x0a04
#define MSG_MON_GNSS		0x0a28
#define MSG_MON_HW		0x0a09
#define MSG_MON_HW2		0x0a0b
#define MSG_MON_IO		0x0a02
#define MSG_MON_MSGPP		0x0a06
#define MSG_MON_PATCH		0x0a27
#define MSG_MON_RXBUF		0x0a07
#define MSG_MON_RXR		0x0a21
#define MSG_MON_SMGR		0x0a2e
#define MSG_MON_TXBUF		0x0a08

#include <string>
#include <chrono>
#include "gnsssatellite.h"

struct UbxMessage {
public:
	uint8_t classID;
	uint8_t messageID;
	uint16_t msgID;
	std::string data;
};

struct gpsTimestamp {
	//   std::chrono::time_point<std::chrono::system_clock> rising_time;
	//   std::chrono::time_point<std::chrono::system_clock> falling_time;
	struct timespec rising_time;
	struct timespec falling_time;
	double accuracy_ns;
	bool valid;
	int channel;
	bool rising;
	bool falling;
	int counter;
};

template <typename T> class gpsProperty{
public:
    gpsProperty() : value() {
        updated=false;
    }
    gpsProperty(const T& val){
        value = val;
        updated = true;
        lastUpdate = std::chrono::system_clock::now();
    }
	std::chrono::time_point<std::chrono::system_clock> lastUpdate;
	std::chrono::duration<double> updatePeriod;
	std::chrono::duration<double> updateAge() { return std::chrono::system_clock::now() - lastUpdate; }
	bool updated;
    gpsProperty& operator=(const T& val) {
        value = val;
		lastUpdate = std::chrono::system_clock::now();
		updated = true;
		return *this;
    }
    const T& operator()() {
		updated = false;
        return value;
    }
private:
    T value;
};

#endif // STRUCTS_AND_DEFINES_H
