#ifndef UBLOX_STRUCTS_H
#define UBLOX_STRUCTS_H

#include "custom_io_operators.h"
#include <QDataStream>
#include <QList>
#include <QString>
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

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
    static constexpr std::array<const char*, last + 1> name { "No Fix", "Dead Reck.", "2D-Fix", "3D-Fix", "GPS+Dead Reck.", "Time Fix", " N/A" };
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

struct GnssPosStruct {
    uint32_t iTOW;
    int32_t lon; // longitude 1e-7 scaling (increase by 1 means 100 nano degrees)
    int32_t lat; // latitude 1e-7 scaling (increase by 1 means 100 nano degrees)
    int32_t height; // height above ellipsoid
    int32_t hMSL; // height above main sea level
    uint32_t hAcc; // horizontal accuracy estimate
    uint32_t vAcc; // vertical accuracy estimate
};

struct GnssConfigStruct {
    uint8_t gnssId;
    uint8_t resTrkCh;
    uint8_t maxTrkCh;
    uint32_t flags;
};

class GnssSatellite {
public:
    GnssSatellite() = default;
    GnssSatellite(int gnssId, int satId, int cnr, int elev, int azim, float prRes, uint32_t flags)
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

    friend QDataStream& operator<<(QDataStream& out, const GnssSatellite& sat);
    friend QDataStream& operator>>(QDataStream& in, GnssSatellite& sat);

public:
    uint8_t GnssId { 0 };
    uint8_t SatId { 0 };
    uint8_t Cnr { 0 };
    int8_t Elev { 0 };
    int16_t Azim { 0 };
    float PrRes { 0. };
    uint8_t Quality { 0 };
    uint8_t Health { 0 };
    uint8_t OrbitSource { 0 };
    bool Used { false };
    bool DiffCorr { false };
    bool Smoothed { false };
};

struct UbxTimePulseStruct {
    enum { ACTIVE = 0x01,
        LOCK_GPS = 0x02,
        LOCK_OTHER = 0x04,
        IS_FREQ = 0x08,
        IS_LENGTH = 0x10,
        ALIGN_TO_TOW = 0x20,
        POLARITY = 0x40,
        GRID_UTC_GPS = 0x780 };
    uint8_t tpIndex = 0;
    uint8_t version = 0;
    int16_t antCableDelay = 0;
    int16_t rfGroupDelay = 0;
    uint32_t freqPeriod = 0;
    uint32_t freqPeriodLock = 0;
    uint32_t pulseLenRatio = 0;
    uint32_t pulseLenRatioLock = 0;
    int32_t userConfigDelay = 0;
    uint32_t flags = 0;
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
    uint32_t accuracy_ns = 0;
    bool valid = false;
    uint8_t timeBase = 0;
    bool utcAvailable = false;
    uint8_t flags = 0;
    uint16_t evtCounter = 0;
};

struct GnssMonHwStruct {
    uint16_t noise { 0 };
    uint16_t agc { 0 };
    uint8_t antStatus { 0 };
    uint8_t antPower { 0 };
    uint8_t jamInd { 0 };
    uint8_t flags { 0 };
};

struct GnssMonHw2Struct {
    int8_t ofsI { 0 };
    int8_t ofsQ { 0 };
    uint8_t magI { 0 };
    uint8_t magQ { 0 };
    uint8_t cfgSrc { 0 };
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

#endif // UBLOX_STRUCTS_H
