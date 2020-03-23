#ifndef GNSSPOSWIDGET_H
#define GNSSPOSWIDGET_H

#include <QWidget>
#include <QMap>
#include <QVector>


class GnssSatellite;


namespace Ui {
class GnssPosWidget;
}

struct SatHistoryPoint {
    QPointF pos;
    QColor color;
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

private:
    Ui::GnssPosWidget *ui;
    QMap<int, QVector<SatHistoryPoint>> satTracks;
    QVector<GnssSatellite> fCurrentSatlist;

    QPointF polar2cartUnity(const QPointF &pol);
};

#endif // GNSSPOSWIDGET_H
