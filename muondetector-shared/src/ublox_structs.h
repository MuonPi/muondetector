#ifndef UBLOX_STRUCTS_H
#define UBLOX_STRUCTS_H

#include <string>
#include <chrono>
#include <gnsssatellite.h>
#include <geodeticpos.h>
#include <sstream>
#include <string>

struct UbxMessage {
public:
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

template <typename T> class gpsProperty {
public:
	gpsProperty() : value() {
		updated = false;
	}
	gpsProperty(const T& val) {
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


struct UbxTimePulseStruct {
	enum { ACTIVE=0x01, LOCK_GPS=0x02, LOCK_OTHER=0x04, IS_FREQ=0x08, IS_LENGTH=0x10, ALIGN_TO_TOW=0x20, POLARITY=0x40, GRID_UTC_GPS=0x780  };
	uint8_t tpIndex=0;
	uint8_t version=0;
	int16_t antCableDelay=0;
	int16_t rfGroupDelay = 0;
	uint32_t freqPeriod = 0;
	uint32_t freqPeriodLock = 0;
	uint32_t pulseLenRatio = 0;
	uint32_t pulseLenRatioLock = 0;
	int32_t userConfigDelay = 0;
	uint32_t flags = 0;
};

struct UbxDopStruct {
	uint16_t gDOP=0, pDOP=0, tDOP=0, vDOP=0, hDOP=0, nDOP=0, eDOP=0;
};

#endif // UBLOX_STRUCTS_H
