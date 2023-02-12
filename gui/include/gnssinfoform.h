#ifndef GNSSINFOFORM_H
#define GNSSINFOFORM_H

#include <QEvent>
#include <QMap>
#include <QVector>
#include <QWidget>

#include <ublox_structs.h>

struct GnssPosStruct;
class GnssSatellite;
struct GnssMonHwStruct;

namespace Ui {
class GnssInfoForm;
}

class GnssInfoForm : public QWidget {
    Q_OBJECT

public:
    explicit GnssInfoForm(QWidget* parent = 0);
    ~GnssInfoForm();

public slots:
    void onSatsReceived(const QVector<GnssSatellite>& satlist);
    void onTimeAccReceived(quint32 acc);
    void onFreqAccReceived(quint32 acc);
    void onIntCounterReceived(quint32 cnt);
    void onGpsMonHWReceived(const GnssMonHwStruct& hwstruct);
    void onGpsMonHW2Received(const GnssMonHw2Struct& hw2struct);
    void onGpsVersionReceived(const QString& swString, const QString& hwString, const QString& protString);
    void onGpsFixReceived(quint8 val);
    void onGeodeticPosReceived(const GnssPosStruct& pos);
    void onUiEnabledStateChange(bool connected);
    void onUbxUptimeReceived(quint32 val);
    void changeEvent(QEvent* e);

private slots:
    void replotIqWidget();

private:
    Ui::GnssInfoForm* ui;
    QVector<QPointF> iqTrack;
    GnssMonHw2Struct fIqData {};
};

#endif // GNSSINFOFORM_H
