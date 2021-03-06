#ifndef GPSSATSFORM_H
#define GPSSATSFORM_H

#include <QWidget>
//#include <gnsssatellite.h>
//#include <geodeticpos.h>
#include <QMap>
#include <QVector>
//#include <muondetector_structs.h>

struct GeodeticPos;
class GnssSatellite;
struct GnssMonHwStruct;
struct GnssMonHw2Struct;

namespace Ui {
class GpsSatsForm;
}

//struct SatHistoryPoint;
/*
struct SatHistoryPoint {
    QPointF pos;
    QColor color;
};
*/
class GpsSatsForm : public QWidget
{
    Q_OBJECT

public:
    explicit GpsSatsForm(QWidget *parent = 0);
    ~GpsSatsForm();

public slots:
    void onSatsReceived(const QVector<GnssSatellite>& satlist);
    void onTimeAccReceived(quint32 acc);
    void onFreqAccReceived(quint32 acc);
    void onIntCounterReceived(quint32 cnt);
//    void onGpsMonHWReceived(quint16 noise, quint16 agc, quint8 antStatus, quint8 antPower, quint8 jamInd, quint8 flags);
    void onGpsMonHWReceived(const GnssMonHwStruct& hwstruct);
//    void onGpsMonHW2Received(qint8 ofsI, quint8 magI, qint8 ofsQ, quint8 magQ, quint8 cfgSrc);
    void onGpsMonHW2Received(const GnssMonHw2Struct& hw2struct);
    void onGpsVersionReceived(const QString& swString, const QString& hwString, const QString& protString);
    void onGpsFixReceived(quint8 val);
    void onGeodeticPosReceived(const GeodeticPos& pos);
    void onUiEnabledStateChange(bool connected);
    void onUbxUptimeReceived(quint32 val);

private:
    Ui::GpsSatsForm *ui;
    QVector<QPointF> iqTrack;
//    QMap<int, QVector<SatHistoryPoint>> satTracks;

};

#endif // GPSSATSFORM_H
