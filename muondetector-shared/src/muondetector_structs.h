#ifndef MUONDETECTOR_STRUCTS_H
#define MUONDETECTOR_STRUCTS_H

#include <muondetector_shared_global.h>
#include <sys/types.h>
#include <string>
#include <QString>
#include <QDataStream>


struct CalibStruct {
public:
    enum {	CALIBFLAGS_NO_CALIB=0x00, CALIBFLAGS_COMPONENTS=0x01,
            CALIBFLAGS_VOLTAGE_COEFFS=0x02, CALIBFLAGS_CURRENT_COEFFS=0x04};

    CalibStruct()=default;
    CalibStruct(const std::string& a_name, const std::string& a_type, uint8_t a_address, const std::string& a_value)
    : name(a_name), type(a_type), address(a_address), value(a_value)
    { }
    ~CalibStruct()=default;
    CalibStruct(const CalibStruct& s)
    : name(s.name), type(s.type), address(s.address), value(s.value)
    { }
    //friend QDataStream& operator << (QDataStream& out, const CalibStruct& calib);
    //friend QDataStream& operator >> (QDataStream& in, CalibStruct& calib);
    std::string name="";
    std::string type="";
    uint16_t address=0;
    std::string value="";
};

//extern inline QDataStream& operator << (QDataStream& out, const CalibStruct& calib);
//extern inline QDataStream& operator >> (QDataStream& in, CalibStruct& calib);
inline QDataStream& operator << (QDataStream& out, const CalibStruct& calib)
{
    out << QString::fromStdString(calib.name) << QString::fromStdString(calib.type)
     << (quint16)calib.address << QString::fromStdString(calib.value);
    return out;
}


inline QDataStream& operator >> (QDataStream& in, CalibStruct& calib)
{
    QString s1,s2,s3;
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

#endif // MUONDETECTOR_STRUCTS_H
