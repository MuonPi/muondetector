#include "gnssposwidget.h"
#include "ui_gnssposwidget.h"
#include <QPainter>
#include <QPixmap>
#include <muondetector_structs.h>
#include <cmath>

using namespace std;

constexpr double pi() { return acos(-1); }
const int MAX_SAT_TRACK_ENTRIES = 10000;
static const QList<QColor> GNSS_COLORS = { Qt::darkGreen, Qt::darkYellow, Qt::blue, Qt::magenta, Qt::gray, Qt::cyan, Qt::red, Qt::black };


GnssPosWidget::GnssPosWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GnssPosWidget)
{
    ui->setupUi(this);
    ui->satLabel->setText(QString::number(ui->satLabel->width())+"x"+QString::number(ui->satLabel->height()));
    connect(ui->receivedSatsCheckBox, &QCheckBox::toggled, this, &GnssPosWidget::replot);
    connect(ui->satLabelsCheckBox, &QCheckBox::toggled, this, &GnssPosWidget::replot);
    connect(ui->tracksCheckBox, &QCheckBox::toggled, this, &GnssPosWidget::replot);
}

GnssPosWidget::~GnssPosWidget()
{
    delete ui;
}

void GnssPosWidget::resizeEvent(QResizeEvent *event)
{
    int len = ui->hostWidget->width();
    if (ui->hostWidget->height()<len) len=ui->hostWidget->height();
    len-=10;
    if (len<=0) len=1;
    ui->satLabel->resize(len,len);
    replot();
    //ui->satLabel->setText(QString::number(ui->satLabel->width())+"x"+QString::number(ui->satLabel->height()));
}

QPointF GnssPosWidget::polar2cartUnity(const QPointF& pol) {
    double magn=(90.-pol.y())/180.;
    double xpos = magn*sin(pi()*pol.x()/180.);
    double ypos = -magn*cos(pi()*pol.x()/180.);
    return QPointF(xpos,ypos);
}

void GnssPosWidget::replot() {
    QVector<GnssSatellite> newlist;
    QString str;
    QColor color;

    const int satPosPixmapSize=ui->satLabel->width();
    const QPointF originOffset(satPosPixmapSize/2., satPosPixmapSize/2.);

    QPixmap satPosPixmap(satPosPixmapSize,satPosPixmapSize);
    //    pixmap.fill(QColor("transparent"));
    satPosPixmap.fill(Qt::white);
    QPainter satPosPainter(&satPosPixmap);
    satPosPainter.setPen(QPen(Qt::black));
    satPosPainter.drawEllipse(QPoint(satPosPixmapSize/2,satPosPixmapSize/2),satPosPixmapSize/6,satPosPixmapSize/6);
    satPosPainter.drawEllipse(QPoint(satPosPixmapSize/2,satPosPixmapSize/2),satPosPixmapSize/3,satPosPixmapSize/3);
    satPosPainter.drawEllipse(QPoint(satPosPixmapSize/2,satPosPixmapSize/2),satPosPixmapSize/2,satPosPixmapSize/2);
    satPosPainter.drawLine(QPoint(satPosPixmapSize/2,0),QPoint(satPosPixmapSize/2,satPosPixmapSize));
    satPosPainter.drawLine(QPoint(0,satPosPixmapSize/2),QPoint(satPosPixmapSize,satPosPixmapSize/2));
    satPosPainter.drawText(satPosPixmapSize/2+2,3,18,18,Qt::AlignHCenter,"N");
    satPosPainter.drawText(satPosPixmapSize/2+2,satPosPixmapSize-19,18,18,Qt::AlignHCenter,"S");
    satPosPainter.drawText(4,satPosPixmapSize/2-19,18,18,Qt::AlignHCenter,"W");
    satPosPainter.drawText(satPosPixmapSize-19,satPosPixmapSize/2-19,18,18,Qt::AlignHCenter,"E");

    QFont font = satPosPainter.font();
    font.setPointSize(font.pointSize()-2);
    satPosPainter.setFont(font);
    satPosPainter.drawText(satPosPixmapSize/2-14,satPosPixmapSize-12,18,18,Qt::AlignHCenter,"0°");
    satPosPainter.drawText(satPosPixmapSize/2-16,satPosPixmapSize*5/6-12,18,18,Qt::AlignHCenter,"30°");
    satPosPainter.drawText(satPosPixmapSize/2-16,satPosPixmapSize*2/3-12,18,18,Qt::AlignHCenter,"60°");


    int nrGoodSats = 0;
    for (auto it=fCurrentSatlist.begin(); it!=fCurrentSatlist.end(); it++)
        if (it->fCnr>0) nrGoodSats++;

//    ui->nrSatsLabel->setText(QString::number(nrGoodSats)+"/"+QString::number(fCurrentSatlist.size()));

    for (int i=0; i<fCurrentSatlist.size(); i++)
    {
        if (ui->receivedSatsCheckBox->isChecked()) {
            if (fCurrentSatlist[i].fCnr>0) newlist.push_back(fCurrentSatlist[i]);
        } else newlist.push_back(fCurrentSatlist[i]);
        if (fCurrentSatlist[i].fElev<=90. && fCurrentSatlist[i].fElev>=-90.) {
            if (ui->receivedSatsCheckBox->isChecked() && fCurrentSatlist[i].fCnr==0) continue;
            /*
            double magn=(90.-fCurrentSatlist[i].fElev)*satPosPixmapSize/180.;
            double xpos = magn*sin(PI*fCurrentSatlist[i].fAzim/180.);
            double ypos = -magn*cos(PI*fCurrentSatlist[i].fAzim/180.);
            */
            QPointF currPos(fCurrentSatlist[i].fAzim, fCurrentSatlist[i].fElev);
            QPointF currPoint = polar2cartUnity(currPos);

            QColor satColor=Qt::white;
            QColor fillColor=Qt::white;
            satColor = GNSS_COLORS[fCurrentSatlist[i].fGnssId];
            /*
            if (fCurrentSatlist[i].fGnssId==0) satColor=QColor(Qt::darkGreen); // GPS
            else if (fCurrentSatlist[i].fGnssId==1) satColor=QColor(Qt::darkYellow); // SBAS
            else if (fCurrentSatlist[i].fGnssId==2) satColor=QColor(Qt::blue);  // GAL
            else if (fCurrentSatlist[i].fGnssId==3) satColor=QColor(Qt::magenta); // BEID
            else if (fCurrentSatlist[i].fGnssId==4) satColor=QColor(Qt::gray); // IMES
            else if (fCurrentSatlist[i].fGnssId==5) satColor=QColor(Qt::cyan); // QZSS
            else if (fCurrentSatlist[i].fGnssId==6) satColor=QColor(Qt::red); // GLNS
            */
            satPosPainter.setPen(satColor);
            fillColor=satColor;
            int alpha = fCurrentSatlist[i].fCnr*255/40;
            if (alpha>255) alpha=255;
            fillColor.setAlpha(alpha);
            int satId = fCurrentSatlist[i].fGnssId*1000 + fCurrentSatlist[i].fSatId;
//            QPointF currPoint(xpos+satPosPixmapSize/2,ypos+satPosPixmapSize/2);
            QPointF lastPoint(-2,-2);
/*
            for (int i=0; i<satTracks[satId].size(); i++) {
                satPosPainter.setPen(satTracks[satId][i].color);
                //satPosPainter.setBrush(satTracks[satId][i].color);
                satPosPainter.drawPoint(satTracks[satId][i].pos);
            }
*/
            if (satTracks[satId].size()) {
                lastPoint=satTracks[satId].last().pos;
            }
            if (lastPoint!=currPoint) {
                SatHistoryPoint p;
                p.pos=currPoint;
                p.color=fillColor;
                satTracks[satId].push_back(p);
            }
            satPosPainter.setPen(satColor);
            satPosPainter.setBrush(fillColor);
/*
            if (fCurrentSatlist[i].fCnr>40) satPosPainter.setBrush(Qt::darkGreen);
            else if (fCurrentSatlist[i].fCnr>30) satPosPainter.setBrush(Qt::green);
            else if (fCurrentSatlist[i].fCnr>20) satPosPainter.setBrush(Qt::yellow);
            else if (fCurrentSatlist[i].fCnr>10) satPosPainter.setBrush(QColor(255,100,0));
            else if (fCurrentSatlist[i].fCnr>0) satPosPainter.setBrush(Qt::red);
            else satPosPainter.setBrush(QColor("transparent"));
*/
            currPoint*=satPosPixmapSize;
            currPoint+=originOffset;

            float satsize=ui->satSizeSpinBox->value();
            satPosPainter.drawEllipse(currPoint,satsize/2.,satsize/2.);
            if (fCurrentSatlist[i].fUsed) satPosPainter.drawEllipse(currPoint,satsize/2.+0.5,satsize/2.+0.5);
            currPoint.rx()+=3;
            if (ui->satLabelsCheckBox->isChecked()) satPosPainter.drawText(currPoint, QString::number(fCurrentSatlist[i].fSatId));
        }
    }

    // draw the sat tracks
    if (ui->tracksCheckBox->isChecked()) {
        foreach (QVector<SatHistoryPoint> sat, satTracks) {
            for (int i=0; i<sat.size(); i++) {
                satPosPainter.setPen(sat[i].color);
                QPointF p(sat[i].pos);
                p*=satPosPixmapSize;
                p+=originOffset;
                //satPosPainter.setBrush(satTracks[satId][i].color);
                satPosPainter.drawPoint(p);
            }
        }
    }

    ui->satLabel->setPixmap(satPosPixmap);
}

void GnssPosWidget::onSatsReceived(const QVector<GnssSatellite> &satlist)
{
    fCurrentSatlist = satlist;
    replot();
}

void GnssPosWidget::on_satSizeSpinBox_valueChanged(int arg1)
{
    replot();
}

void GnssPosWidget::onUiEnabledStateChange(bool connected)
{
    if (!connected) {
        QVector<GnssSatellite> emptylist;
//        onSatsReceived(emptylist);
        fCurrentSatlist.clear();
    }
    this->setEnabled(connected);

}
