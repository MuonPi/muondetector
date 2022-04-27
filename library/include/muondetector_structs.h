#ifndef MUONDETECTOR_STRUCTS_H
#define MUONDETECTOR_STRUCTS_H

#include "gpio_pin_definitions.h"
#include "histogram.h"
#include "muondetector_shared_global.h"
#include <config.h>

#include <QDataStream>
#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <sys/types.h>

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

inline QDataStream& operator<<(QDataStream& out, const CalibStruct& calib)
{
    out << QString::fromStdString(calib.name) << QString::fromStdString(calib.type)
        << (quint16)calib.address << QString::fromStdString(calib.value);
    return out;
}

inline QDataStream& operator>>(QDataStream& in, CalibStruct& calib)
{
    QString s1, s2, s3;
    quint16 u;
    in >> s1 >> s2;
    in >> u;
    in >> s3;
    calib.name = s1.toStdString();
    calib.type = s2.toStdString();
    calib.address = (uint16_t)u;
    calib.value = s3.toStdString();
    return in;
}

inline QDataStream& operator>>(QDataStream& in, Histogram& h)
{
    h.clear();
    QString name, unit;
    in >> name >> h.fMin >> h.fMax >> h.fUnderflow >> h.fOverflow >> h.fNrBins;
    h.setName(name.toStdString());
    for (int i = 0; i < h.fNrBins; i++) {
        in >> h.fHistogramMap[i];
    }
    in >> unit;
    h.setUnit(unit.toStdString());
    return in;
}

inline QDataStream& operator<<(QDataStream& out, const Histogram& h)
{
    out << QString::fromStdString(h.fName) << h.fMin << h.fMax << h.fUnderflow << h.fOverflow << h.fNrBins;
    for (int i = 0; i < h.fNrBins; i++) {
        out << h.getBinContent(i);
    }
    out << QString::fromStdString(h.fUnit);
    return out;
}

inline QDataStream& operator>>(QDataStream& in, LogInfoStruct& lis)
{
    qint32 logRotation;
    qint32 logAge;
    quint8 status;
    in >> lis.logFileName >> lis.dataFileName >> status >> lis.logFileSize
        >> lis.dataFileSize >> logAge >> logRotation >> lis.logEnabled;
    lis.logAge = std::chrono::seconds(logAge);
    lis.logRotationDuration = std::chrono::seconds(logRotation);
    lis.status = static_cast<LogInfoStruct::status_t>(status);
    return in;
}

inline QDataStream& operator<<(QDataStream& out, const LogInfoStruct& lis)
{
    out << lis.logFileName << lis.dataFileName << static_cast<quint8>(lis.status) << lis.logFileSize
        << lis.dataFileSize << static_cast<qint32>(lis.logAge.count()) << static_cast<qint32>(lis.logRotationDuration.count()) << lis.logEnabled;
    return out;
}

inline QDataStream& operator>>(QDataStream& in, MuonPi::Version::Version& ver)
{
    qint16 major, minor, patch;
    in >> major >> minor >> patch;
    QString additional, hash;
    in >> additional >> hash;
    ver = MuonPi::Version::Version { major, minor, patch, additional.toStdString(), hash.toStdString() };
    return in;
}

inline QDataStream& operator<<(QDataStream& out, const MuonPi::Version::Version& ver)
{
    out << (qint16)ver.major << (qint16)ver.minor << (qint16)ver.patch << QString::fromStdString(ver.additional) << QString::fromStdString(ver.hash);
    return out;
}

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

#endif // MUONDETECTOR_STRUCTS_H
