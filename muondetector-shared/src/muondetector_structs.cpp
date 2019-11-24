#include <muondetector_structs.h>
#include <QDataStream>
#include <QString>


//const std::string GnssSatellite::GNSS_ID_STRING[] = { " GPS","SBAS"," GAL","BEID","IMES","QZSS","GLNS"," N/A" };

/*
QDataStream& operator << (QDataStream& out, const CalibStruct& calib)
{
    out << QString::fromStdString(calib.name) << QString::fromStdString(calib.type)
     << (quint16)calib.address << QString::fromStdString(calib.value);
    return out;
}


QDataStream& operator >> (QDataStream& in, CalibStruct& calib)
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
*/
