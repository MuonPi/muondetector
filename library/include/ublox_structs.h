#ifndef UBLOX_STRUCTS_H
#define UBLOX_STRUCTS_H

#include <chrono>
#include <sstream>
#include <string>

#include "muondetector_structs.h"

struct GeodeticPos;
class GnssSatellite;

struct UbxMessage {
public:
    UbxMessage() = default;
    std::uint16_t full_id { 0 };
    std::string data {};
    [[nodiscard]] auto class_id() const -> std::uint8_t { return (full_id >> 8) & 0xff; }
    [[nodiscard]] auto message_id() const -> std::uint8_t { return full_id & 0xff; }
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

template <typename T>
class gpsProperty {
public:
    gpsProperty()
        : value()
    {
        updated = false;
    }
    gpsProperty(const T& val)
    {
        value = val;
        updated = true;
        lastUpdate = std::chrono::system_clock::now();
    }
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;
    std::chrono::duration<double> updatePeriod;
    std::chrono::duration<double> updateAge() { return std::chrono::system_clock::now() - lastUpdate; }
    bool updated;
    gpsProperty& operator=(const T& val)
    {
        value = val;
        lastUpdate = std::chrono::system_clock::now();
        updated = true;
        return *this;
    }
    const T& operator()()
    {
        updated = false;
        return value;
    }

private:
    T value;
};

struct UbxDopStruct {
    uint16_t gDOP = 0, pDOP = 0, tDOP = 0, vDOP = 0, hDOP = 0, nDOP = 0, eDOP = 0;
};

#endif // UBLOX_STRUCTS_H
