#ifndef GNSSPOSWIDGET_H
#define GNSSPOSWIDGET_H

#include <QWidget>
#include <QMap>
#include <QVector>
#include <QPixmap>
#include <QDateTime>


class GnssSatellite;


namespace Ui {
class GnssPosWidget;
}

struct SatHistoryPoint {
    QPointF posCart;
    QPoint posPolar;
    quint8 satId;
    quint8 gnssId;
    quint8 cnr;
    QColor color;
    QDateTime time;
};

class GnssPosWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GnssPosWidget(QWidget *parent = 0);
    ~GnssPosWidget();
    void resizeEvent(QResizeEvent *event);

public slots:
    void onSatsReceived(const QVector<GnssSatellite> &satlist);
    void replot();
    void onUiEnabledStateChange(bool connected);

private slots:
    void on_satSizeSpinBox_valueChanged(int arg1);
    void popUpMenu(const QPoint &);
    void exportToFile();

private:
    Ui::GnssPosWidget *ui;
    QMap<int, QVector<SatHistoryPoint>> satTracks;
    QVector<GnssSatellite> fCurrentSatlist;

    QPointF polar2cartUnity(const QPointF &pol);
    void drawPolarPixMap(QPixmap& pm);
    void drawCartesianPixMap(QPixmap& pm);
};

#endif // GNSSPOSWIDGET_H
