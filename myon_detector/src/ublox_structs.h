#ifndef UBLOX_STRUCTS_H
#define UBLOX_STRUCTS_H

#include <string>
#include <chrono>
#include <gnsssatellite.h>
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

#endif // UBLOX_STRUCTS_H
