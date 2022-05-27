#include "custom_io_operators.h"
#include "config.h"
#include "muondetector_structs.h"
#include "ublox_structs.h"

#include <iomanip>

std::ostream& operator<<(std::ostream& os, const QString& someQString)
{
    os << someQString.toStdString();
    return os;
}

std::ostream& operator<<(std::ostream& os, const std::chrono::time_point<std::chrono::system_clock>& timestamp)
{
    std::chrono::microseconds mus = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch());
    std::chrono::seconds secs = std::chrono::duration_cast<std::chrono::seconds>(mus);
    std::chrono::microseconds subs = mus - secs;

    os << secs.count() << "." << std::setw(6) << std::setfill('0') << subs.count() << " " << std::setfill(' ');
    return os;
}

std::ostream& operator<<(std::ostream& os, const timespec& ts)
{
    os << ts.tv_sec << "." << std::setw(9) << std::setfill('0') << ts.tv_nsec << " " << std::setfill(' ');
    return os;
}

QDataStream& operator>>(QDataStream& in, GnssSatellite& sat)
{
    int gnssid {}, satid {}, cnr {}, elev {}, azim {}, quality {}, health {}, orbitsource {};

    in >> gnssid >> satid >> cnr >> elev >> azim
        >> sat.PrRes >> quality >> health >> orbitsource
        >> sat.Used >> sat.DiffCorr >> sat.Smoothed;
    sat.GnssId = gnssid;
    sat.SatId = satid;
    sat.Cnr = cnr;
    sat.Elev = elev;
    sat.Azim = azim;
    sat.Quality = quality;
    sat.Health = health;
    sat.OrbitSource = orbitsource;
    return in;
}

QDataStream& operator<<(QDataStream& out, const GnssSatellite& sat)
{
    out << static_cast<int>(sat.GnssId)
        << static_cast<int>(sat.SatId)
        << static_cast<int>(sat.Cnr)
        << static_cast<int>(sat.Elev)
        << static_cast<int>(sat.Azim)
        << sat.PrRes
        << static_cast<int>(sat.Quality)
        << static_cast<int>(sat.Health)
        << static_cast<int>(sat.OrbitSource)
        << sat.Used << sat.DiffCorr << sat.Smoothed;
    return out;
}

QDataStream& operator<<(QDataStream& out, const GnssPosStruct& pos)
{
    out << pos.iTOW << pos.lon << pos.lat
        << pos.height << pos.hMSL << pos.hAcc
        << pos.vAcc;
    return out;
}

QDataStream& operator>>(QDataStream& in, UbxTimePulseStruct& tp)
{
    in >> tp.tpIndex >> tp.version >> tp.antCableDelay >> tp.rfGroupDelay
        >> tp.freqPeriod >> tp.freqPeriodLock >> tp.pulseLenRatio >> tp.pulseLenRatioLock
        >> tp.userConfigDelay >> tp.flags;
    return in;
}

QDataStream& operator<<(QDataStream& out, const UbxTimePulseStruct& tp)
{
    out << tp.tpIndex << tp.version << tp.antCableDelay << tp.rfGroupDelay
        << tp.freqPeriod << tp.freqPeriodLock << tp.pulseLenRatio << tp.pulseLenRatioLock
        << tp.userConfigDelay << tp.flags;
    return out;
}

QDataStream& operator>>(QDataStream& in, UbxTimeMarkStruct& tm)
{
    qint64 sec, nsec;
    in >> sec >> nsec;
    tm.rising.tv_sec = sec;
    tm.rising.tv_nsec = nsec;
    in >> sec >> nsec;
    tm.falling.tv_sec = sec;
    tm.falling.tv_nsec = nsec;
    in >> tm.risingValid >> tm.fallingValid >> tm.accuracy_ns >> tm.valid
        >> tm.timeBase >> tm.utcAvailable >> tm.flags >> tm.evtCounter;
    return in;
}

QDataStream& operator<<(QDataStream& out, const UbxTimeMarkStruct& tm)
{
    out << (qint64)tm.rising.tv_sec << (qint64)tm.rising.tv_nsec << (qint64)tm.falling.tv_sec << (qint64)tm.falling.tv_nsec
        << tm.risingValid << tm.fallingValid << tm.accuracy_ns << tm.valid
        << tm.timeBase << tm.utcAvailable << tm.flags << tm.evtCounter;
    return out;
}

QDataStream& operator>>(QDataStream& in, GnssMonHwStruct& hw)
{
    in >> hw.noise >> hw.agc >> hw.antStatus >> hw.antPower >> hw.jamInd >> hw.flags;
    return in;
}

QDataStream& operator<<(QDataStream& out, const GnssMonHwStruct& hw)
{
    out << hw.noise << hw.agc << hw.antStatus << hw.antPower << hw.jamInd << hw.flags;
    return out;
}

QDataStream& operator>>(QDataStream& in, GnssMonHw2Struct& hw2)
{
    in >> hw2.ofsI >> hw2.magI >> hw2.ofsQ >> hw2.magQ >> hw2.cfgSrc;
    return in;
}

QDataStream& operator<<(QDataStream& out, const GnssMonHw2Struct& hw2)
{
    out << hw2.ofsI << hw2.magI << hw2.ofsQ << hw2.magQ << hw2.cfgSrc;
    return out;
}

QDataStream& operator<<(QDataStream& out, const CalibStruct& calib)
{
    out << QString::fromStdString(calib.name) << QString::fromStdString(calib.type)
        << (quint16)calib.address << QString::fromStdString(calib.value);
    return out;
}

QDataStream& operator>>(QDataStream& in, CalibStruct& calib)
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

QDataStream& operator>>(QDataStream& in, Histogram& h)
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

QDataStream& operator<<(QDataStream& out, const Histogram& h)
{
    out << QString::fromStdString(h.fName) << h.fMin << h.fMax << h.fUnderflow << h.fOverflow << h.fNrBins;
    for (int i = 0; i < h.fNrBins; i++) {
        out << h.getBinContent(i);
    }
    out << QString::fromStdString(h.fUnit);
    return out;
}

QDataStream& operator>>(QDataStream& in, LogInfoStruct& lis)
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

QDataStream& operator<<(QDataStream& out, const LogInfoStruct& lis)
{
    out << lis.logFileName << lis.dataFileName << static_cast<quint8>(lis.status) << lis.logFileSize
        << lis.dataFileSize << static_cast<qint32>(lis.logAge.count()) << static_cast<qint32>(lis.logRotationDuration.count()) << lis.logEnabled;
    return out;
}

QDataStream& operator>>(QDataStream& in, MuonPi::Version::Version& ver)
{
    qint16 major, minor, patch;
    in >> major >> minor >> patch;
    QString additional, hash;
    in >> additional >> hash;
    ver = MuonPi::Version::Version { major, minor, patch, additional.toStdString(), hash.toStdString() };
    return in;
}

QDataStream& operator<<(QDataStream& out, const MuonPi::Version::Version& ver)
{
    out << (qint16)ver.major << (qint16)ver.minor << (qint16)ver.patch << QString::fromStdString(ver.additional) << QString::fromStdString(ver.hash);
    return out;
}
