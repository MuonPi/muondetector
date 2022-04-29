#ifndef CUSTOM_IO_OPERATORS_H
#define CUSTOM_IO_OPERATORS_H
#include <QString>
#include <chrono>
#include <iostream>
#include <QDataStream>

class GnssSatellite;
struct UbxTimeMarkStruct;
struct UbxTimePulseStruct;
struct GnssMonHwStruct;
struct GnssMonHw2Struct;
struct CalibStruct;
class Histogram;
struct LogInfoStruct;
namespace MuonPi::Version {
    struct Version;
}
std::ostream& operator<<(std::ostream& os, const QString& someQString);
std::ostream& operator<<(std::ostream& os, const std::chrono::time_point<std::chrono::system_clock>& timestamp);
std::ostream& operator<<(std::ostream& os, const timespec& ts);

QDataStream& operator>>(QDataStream& in, GnssSatellite& sat);
QDataStream& operator<<(QDataStream& out, const GnssSatellite& sat);

QDataStream& operator>>(QDataStream& in, UbxTimePulseStruct& tp);
QDataStream& operator<<(QDataStream& out, const UbxTimePulseStruct& tp);

QDataStream& operator>>(QDataStream& in, UbxTimeMarkStruct& tm);
QDataStream& operator<<(QDataStream& out, const UbxTimeMarkStruct& tm);

QDataStream& operator>>(QDataStream& in, GnssMonHwStruct& hw);
QDataStream& operator<<(QDataStream& out, const GnssMonHwStruct& hw);
QDataStream& operator>>(QDataStream& in, GnssMonHw2Struct& hw2);
QDataStream& operator<<(QDataStream& out, const GnssMonHw2Struct& hw2);

QDataStream& operator<<(QDataStream& out, const CalibStruct& calib);
QDataStream& operator>>(QDataStream& in, CalibStruct& calib);
QDataStream& operator>>(QDataStream& in, Histogram& h);
QDataStream& operator<<(QDataStream& out, const Histogram& h);
QDataStream& operator>>(QDataStream& in, LogInfoStruct& lis);
QDataStream& operator<<(QDataStream& out, const LogInfoStruct& lis);
QDataStream& operator>>(QDataStream& in, MuonPi::Version::Version& ver);
QDataStream& operator<<(QDataStream& out, const MuonPi::Version::Version& ver);

#endif //CUSTOM_IO_OPERATORS_H
