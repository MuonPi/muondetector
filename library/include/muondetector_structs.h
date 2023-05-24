#ifndef MUONDETECTOR_STRUCTS_H
#define MUONDETECTOR_STRUCTS_H

#include "config.h"
#include "custom_io_operators.h"
#include "gpio_pin_definitions.h"
#include "histogram.h"
#include "muondetector_shared_global.h"

#include "ublox_structs.h"
#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>
#include <any>
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <sys/types.h>

using EventTime = std::chrono::time_point<std::chrono::system_clock>;

enum class ADC_SAMPLING_MODE {
    DISABLED = 0,
    PEAK = 1,
    TRACE = 2
};

class GeneralEvent {
public:
    explicit GeneralEvent(std::function<void()> fn);
public slots:
    void trigger();

protected:
    std::function<void()> fFn;
};

struct GeoPosition {
    double longitude { 0. };
    double latitude { 0. };
    double altitude { 0. };
    double hor_error { 0. };
    double vert_error { 0. };
    bool valid() const { return !(longitude == 0. && latitude == 0. && altitude == 0. && hor_error == 0. && vert_error == 0.); }

    [[nodiscard]] auto pos_error() const -> double { return std::sqrt(hor_error * hor_error + vert_error * vert_error); }
    [[nodiscard]] auto getPosStruct() const -> GnssPosStruct
    {
        GnssPosStruct pos_struct {
            0,
            static_cast<int32_t>(longitude * 1e7),
            static_cast<int32_t>(latitude * 1e7),
            static_cast<int32_t>(altitude * 1e3),
            static_cast<int32_t>(altitude * 1e3),
            static_cast<uint32_t>(hor_error * 1e3),
            static_cast<uint32_t>(vert_error * 1e3)
        };
        return pos_struct;
    }
    friend bool operator==(const GeoPosition& a, const GeoPosition& b)
    {
        return (a.longitude == b.longitude && a.latitude == b.latitude && a.altitude == b.altitude && a.hor_error == b.hor_error && a.vert_error == b.vert_error);
    }
    friend bool operator!=(const GeoPosition& a, const GeoPosition& b)
    {
        return !(a == b);
    }
};

struct PositionModeConfig {
    enum class Mode {
        Static = 0,
        LockIn = 1,
        Auto = 2,
        first = Static,
        last = Auto
    } mode;
    GeoPosition static_position {};
    double lock_in_max_dop { 3. };
    double lock_in_min_error_meters { 7.5 };
    enum class FilterType {
        None = 0,
        Kalman = 1,
        HistoMean = 2,
        HistoMedian = 3,
        HistoMpv = 4,
        first = None,
        last = HistoMpv
    } filter_config;
    friend bool operator==(const PositionModeConfig& a, const PositionModeConfig& b)
    {
        return (a.mode == b.mode && a.filter_config == b.filter_config && a.lock_in_max_dop == b.lock_in_max_dop && a.lock_in_min_error_meters == b.lock_in_min_error_meters && a.static_position == b.static_position);
    }
    friend bool operator!=(const PositionModeConfig& a, const PositionModeConfig& b)
    {
        return !(a == b);
    }
    static constexpr std::array<const char*, static_cast<size_t>(Mode::last) + 1> mode_name { "static", "lock-in", "auto" };
    static constexpr std::array<const char*, static_cast<size_t>(FilterType::last) + 1> filter_name { "none", "kalman", "histo_mean", "histo_median", "histo_mpv" };
};

struct IoConfiguration {
    TIMING_MUX_SELECTION timing_input { TIMING_MUX_SELECTION::UNDEFINED };
    GPIO_SIGNAL event_trigger { GPIO_SIGNAL::UNDEFINED_PIN };
    GeneralEvent led1_event;
    GeneralEvent led2_event;
};

struct CalibStruct {
public:
    enum {
        CALIBFLAGS_NO_CALIB = 0x00,
        CALIBFLAGS_COMPONENTS = 0x01,
        CALIBFLAGS_VOLTAGE_COEFFS = 0x02,
        CALIBFLAGS_CURRENT_COEFFS = 0x04
    };

    enum {
        FEATUREFLAGS_NONE = 0x00,
        FEATUREFLAGS_GNSS = 0x01,
        FEATUREFLAGS_ENERGY = 0x02,
        FEATUREFLAGS_DETBIAS = 0x04,
        FEATUREFLAGS_PREAMP_BIAS = 0x08,
        FEATUREFLAGS_DUAL_CHANNEL = 0x10
    };

    CalibStruct() = default;
    CalibStruct(const std::string& a_name, const std::string& a_type, uint8_t a_address, const std::string& a_value)
        : name(a_name)
        , type(a_type)
        , address(a_address)
        , value(a_value)
    {
    }
    ~CalibStruct() = default;
    CalibStruct(const CalibStruct& s)
        : name(s.name)
        , type(s.type)
        , address(s.address)
        , value(s.value)
    {
    }
    std::string name = "";
    std::string type = "";
    uint16_t address = 0;
    std::string value = "";
};

enum I2C_DEVICE_MODE { I2C_MODE_NONE = 0,
    I2C_MODE_NORMAL = 0x01,
    I2C_MODE_FORCE = 0x02,
    I2C_MODE_UNREACHABLE = 0x04,
    I2C_MODE_FAILED = 0x08,
    I2C_MODE_LOCKED = 0x10 };

struct I2cDeviceEntry {
    quint8 address;
    QString name;
    quint8 status;
    quint32 nrBytesWritten;
    quint32 nrBytesRead;
    quint32 nrIoErrors;
    quint32 lastTransactionTime; // in us
};

struct LogInfoStruct {
    QString logFileName;
    QString dataFileName;
    enum status_t : quint8 {
        ERROR = 0,
        NORMAL,
        LOG_ONLY,
        OFF
    } status;
    quint32 logFileSize;
    quint32 dataFileSize;
    std::chrono::seconds logAge;
    std::chrono::seconds logRotationDuration { 86400L };
    bool logEnabled { true };
};

struct OledItem {
    QString name;
    QString displayString;
};

static const QMap<quint8, QString> I2C_MODE_STRINGMAP = { { 0x00, "None" },
    { 0x01, "Normal" },
    { 0x02, "System" },
    { 0x04, "Unreachable" },
    { 0x08, "Failed" },
    { 0x10, "Locked" } };
/*
class Property {
public:
    Property() = default;

    template <typename T>
    Property(const T& val)
        : name("")
        , unit("")
    {
        typeId = qMetaTypeId<T>();
        if (typeId == QMetaType::UnknownType) {
            typeId = qRegisterMetaType<T>();
        }
        value = QVariant(val);
        updated = true;
    }

    template <typename T>
    Property(const QString& a_name, const T& val, const QString& a_unit = "")
        : name(a_name)
        , unit(a_unit)
    {
        typeId = qMetaTypeId<T>();
        if (typeId == QMetaType::UnknownType) {
            typeId = qRegisterMetaType<T>();
        }
        value = QVariant(val);
        updated = true;
    }

    Property(const Property& prop) = default;
    Property& operator=(const Property& prop)
    {
        name = prop.name;
        unit = prop.unit;
        value = prop.value;
        typeId = prop.typeId;
        updated = prop.updated;
        return *this;
    }

    Property& operator=(const QVariant& val)
    {
        value = val;
        //lastUpdate = std::chrono::system_clock::now();
        updated = true;
        return *this;
    }

    const QVariant& operator()()
    {
        updated = false;
        return value;
    }

    bool isUpdated() const { return updated; }
    int type() const { return typeId; }

    QString name = "";
    QString unit = "";

private:
    QVariant value;
    bool updated = false;
    int typeId = 0;
};
*/

/*
class Property {
public:
    Property() = default;

    template <class T>
    Property(const T& val)
        : name("")
        , unit("")
    {
        m_value = val;
        m_updated = true;
        m_update_time = std::chrono::steady_clock::now();
    }

    template <class T>
    Property(const std::string& a_name, const T& val, const std::string& a_unit = "")
        : name(a_name)
        , unit(a_unit)
    {
        m_value = val;
        m_updated = true;
        m_update_time = std::chrono::steady_clock::now();
    }

    Property(const Property& prop) = default;
    Property& operator=(const Property& prop)
    {
        name = prop.name;
        unit = prop.unit;
        m_value = prop.m_value;
        m_updated = prop.m_updated;
        m_update_time = std::chrono::steady_clock::now();
        return *this;
    }

    template <class T>
    Property& operator=(const T& val)
    {
        m_value = val;
        m_updated = true;
        m_update_time = std::chrono::steady_clock::now();
        return *this;
    }

    template <class T>
    const T operator()()
    {
        m_updated = false;
        return std::any_cast<T>(m_value);
    }

    template <class T>
    const T get()
    {
        m_updated = false;
        return std::any_cast<T>(m_value);
    }

    bool updated() const { return m_updated; }
    [[nodiscard]] auto age() const -> std::chrono::microseconds {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_update_time);
    }

    std::string name { };
    std::string unit { };

private:
    std::any m_value {};
    bool m_updated { false };
    std::chrono::time_point<std::chrono::steady_clock> m_update_time { };
};
*/

template <typename T>
class Property {
public:
    Property() = default;

    Property(const std::string& a_name, const T& val)
        : name(a_name)
    {
        m_value = val;
        m_updated = true;
        m_update_time = std::chrono::steady_clock::now();
    }

    Property(const Property& prop) = default;

    Property<T>& operator=(const T& val)
    {
        m_value = val;
        m_updated = true;
        m_update_time = std::chrono::steady_clock::now();
        return *this;
    }

    const T& operator()()
    {
        m_updated = false;
        return m_value;
    }

    const T& get()
    {
        m_updated = false;
        return m_value;
    }

    bool updated() const { return m_updated; }
    [[nodiscard]] auto age() const -> std::chrono::microseconds
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - m_update_time);
    }

    std::string name {};

protected:
    T m_value {};
    bool m_updated { false };
    std::chrono::time_point<std::chrono::steady_clock> m_update_time {};
};

#endif // MUONDETECTOR_STRUCTS_H
