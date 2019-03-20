#include <cmath>
#include "gpssatsform.h"
#include "ui_gpssatsform.h"
#include <QPainter>
#include <QPixmap>

const int MAX_IQTRACK_BUFFER = 250;

const static double PI = 3.1415926535;

const QVector<QString> FIX_TYPE_STRINGS = { "No Fix", "Dead Reck." , "2D-Fix", "3D-Fix", "GPS+Dead Reck.", "Time Fix"  };
const QString GNSS_ID_STRING[] = { " GPS","SBAS"," GAL","BEID","IMES","QZSS","GLNS"," N/A" };
const QString GNSS_ORBIT_SRC_STRING[] = { "N/A","Ephem","Alm","AOP","AOP+","Alt","Alt","Alt" };

// helper function to format human readable numbers with common suffixes (k(ilo), M(ega), m(illi) etc.)
QString printReadableFloat(double value, int prec=2, int lowOrderInhibit=-12, int highOrderInhibit=9) {
    QString str="";
    QString suffix="";
    double order=std::log10(std::fabs(value));
    if (order>=lowOrderInhibit && order<=highOrderInhibit) {
    if (order>=9) { value*=1e-9; suffix="G"; }
    else if (order>=6) { value*=1e-6; suffix="M"; }
    else if (order>=3) { value*=1e-3; suffix="k"; }
    else if (order>=0) { suffix=""; }
    //else if (order>-2) { value*=100.; suffix="c"; }
    else if (order>=-3) { value*=1000.; suffix="m"; }
    else if (order>=-6) { value*=1e6; suffix="u"; }
    else if (order>=-9) { value*=1e9; suffix="n"; }
    else if (order>=-12) { value*=1e12; suffix="p"; }
    }
    char fmtChar='f';
    double newOrder = std::log10(std::fabs(value));
    if (fabs(newOrder)>=3.) { fmtChar='g'; }
    else { prec=prec-(int)newOrder - 1;  }
    if (prec<0) prec=0;
    return QString::number(value,fmtChar,prec)+" "+suffix;
}


GpsSatsForm::GpsSatsForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GpsSatsForm)
{
    ui->setupUi(this);
    ui->satsTableWidget->resizeColumnsToContents();
    //ui->satsTableWidget->resizeRowsToContents();
}

GpsSatsForm::~GpsSatsForm()
{
    delete ui;
}

void GpsSatsForm::onSatsReceived(const QVector<GnssSatellite> &satlist)
{
    QVector<GnssSatellite> newlist;
    QString str;
    QColor color;
    const int iqPixmapSize=105;
    const int satPosPixmapSize=220;
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

    int nrGoodSats = 0;
    for (auto it=satlist.begin(); it!=satlist.end(); it++)
        if (it->fCnr>0) nrGoodSats++;

    ui->nrSatsLabel->setText(QString::number(nrGoodSats)+"/"+QString::number(satlist.size()));

    for (int i=0; i<satlist.size(); i++)
    {
        if (ui->visibleSatsCheckBox->isChecked()) {
            if (satlist[i].fCnr>0) newlist.push_back(satlist[i]);
        } else newlist.push_back(satlist[i]);
        if (satlist[i].fElev<=90. && satlist[i].fElev>=-90.) {
            if (ui->visibleSatsCheckBox->isChecked() && satlist[i].fCnr==0) continue;
            double magn=(90.-satlist[i].fElev)*satPosPixmapSize/180.;
            double xpos = magn*sin(PI*satlist[i].fAzim/180.);
            double ypos = -magn*cos(PI*satlist[i].fAzim/180.);
            QColor satColor=Qt::white;
            QColor fillColor=Qt::white;
            if (satlist[i].fGnssId==0) satColor=QColor(Qt::darkGreen); // GPS
            else if (satlist[i].fGnssId==1) satColor=QColor(Qt::darkYellow); // SBAS
            else if (satlist[i].fGnssId==2) satColor=QColor(Qt::blue);  // GAL
            else if (satlist[i].fGnssId==3) satColor=QColor(Qt::magenta); // BEID
            else if (satlist[i].fGnssId==4) satColor=QColor(Qt::gray); // IMES
            else if (satlist[i].fGnssId==5) satColor=QColor(Qt::cyan); // QZSS
            else if (satlist[i].fGnssId==6) satColor=QColor(Qt::red); // GLNS
            satPosPainter.setPen(satColor);
            fillColor=satColor;
            int alpha = satlist[i].fCnr*255/40;
            if (alpha>255) alpha=255;
            fillColor.setAlpha(alpha);
            int satId = satlist[i].fGnssId*1000 + satlist[i].fSatId;
            QPointF currPoint(xpos+satPosPixmapSize/2,ypos+satPosPixmapSize/2);
            QPointF lastPoint;
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
            if (satlist[i].fCnr>40) satPosPainter.setBrush(Qt::darkGreen);
            else if (satlist[i].fCnr>30) satPosPainter.setBrush(Qt::green);
            else if (satlist[i].fCnr>20) satPosPainter.setBrush(Qt::yellow);
            else if (satlist[i].fCnr>10) satPosPainter.setBrush(QColor(255,100,0));
            else if (satlist[i].fCnr>0) satPosPainter.setBrush(Qt::red);
            else satPosPainter.setBrush(QColor("transparent"));
*/
            satPosPainter.drawEllipse(currPoint,3.,3.);
            if (satlist[i].fUsed) satPosPainter.drawEllipse(currPoint,3.5,3.5);
        }
    }

    // draw the sat tracks
    foreach (QVector<SatHistoryPoint> sat, satTracks) {
        for (int i=0; i<sat.size(); i++) {
            satPosPainter.setPen(sat[i].color);
            //satPosPainter.setBrush(satTracks[satId][i].color);
            satPosPainter.drawPoint(sat[i].pos);
        }
    }

    ui->satPosLabel->setPixmap(satPosPixmap);

    int N=newlist.size();
    ui->satsTableWidget->setRowCount(N);
    for (int i=0; i<N; i++)
    {
        QTableWidgetItem *newItem1 = new QTableWidgetItem(QString::number(newlist[i].fSatId));
        newItem1->setSizeHint(QSize(25,24));
        ui->satsTableWidget->setItem(i, 0, newItem1);
//        QTableWidgetItem *newItem2 = new QTableWidgetItem(QString::number(satlist[i].fGnssId));
        QTableWidgetItem *newItem2 = new QTableWidgetItem(GNSS_ID_STRING[newlist[i].fGnssId]);
        newItem2->setSizeHint(QSize(50,24));
        ui->satsTableWidget->setItem(i, 1, newItem2);
        QTableWidgetItem *newItem3 = new QTableWidgetItem(QString::number(newlist[i].fCnr));
        newItem3->setSizeHint(QSize(70,24));
        ui->satsTableWidget->setItem(i, 2, newItem3);
        QTableWidgetItem *newItem4 = new QTableWidgetItem(QString::number(newlist[i].fAzim));
        newItem4->setSizeHint(QSize(100,24));
        ui->satsTableWidget->setItem(i, 3, newItem4);
        QTableWidgetItem *newItem5 = new QTableWidgetItem(QString::number(newlist[i].fElev));
        newItem5->setSizeHint(QSize(100,24));
        ui->satsTableWidget->setItem(i, 4, newItem5);
        QTableWidgetItem *newItem6 = new QTableWidgetItem(printReadableFloat(newlist[i].fPrRes,2,0));
        newItem6->setSizeHint(QSize(60,24));
        ui->satsTableWidget->setItem(i, 5, newItem6);
        QTableWidgetItem *newItem7 = new QTableWidgetItem(QString::number(newlist[i].fQuality));
        color=Qt::green;
        float transp=0.166*(newlist[i].fQuality-1);
        if (transp<0.) transp=0.;
        if (transp>1.) transp=1.;
        color.setAlphaF(transp);
        newItem7->setBackgroundColor(color);

        newItem7->setSizeHint(QSize(25,24));
        ui->satsTableWidget->setItem(i, 6, newItem7);
        if (newlist[i].fHealth==0) { str="N/A"; color=Qt::lightGray; }
        else if (newlist[i].fHealth==1) { str="good"; color=Qt::green; }
        else if (newlist[i].fHealth>=2) { str="bad"; color=Qt::red; }
        QTableWidgetItem *newItem8 = new QTableWidgetItem(str);
        //newItem8->setBackgroundColor(color);
        newItem8->setSizeHint(QSize(25,24));
        ui->satsTableWidget->setItem(i, 7, newItem8);
        int orbSrc=newlist[i].fOrbitSource;
        if (orbSrc<0) orbSrc=0;
        if (orbSrc>7) orbSrc=7;
        QTableWidgetItem *newItem9 = new QTableWidgetItem(GNSS_ORBIT_SRC_STRING[orbSrc]);
        newItem9->setSizeHint(QSize(50,24));
        ui->satsTableWidget->setItem(i, 8, newItem9);
        QTableWidgetItem *newItem10 = new QTableWidgetItem();
        newItem10->setCheckState(Qt::CheckState::Unchecked);
        if (newlist[i].fUsed) newItem10->setCheckState(Qt::CheckState::Checked);
        newItem10->setFlags(newItem10->flags() & (~Qt::ItemIsUserCheckable)); // disables checkbox edit from user
        newItem10->setFlags(newItem10->flags() & (~Qt::ItemIsEditable));
        newItem10->setSizeHint(QSize(20,24));
        ui->satsTableWidget->setItem(i, 9, newItem10);
        QTableWidgetItem *newItem11 = new QTableWidgetItem();
        newItem11->setCheckState(Qt::CheckState::Unchecked);
        if (newlist[i].fDiffCorr) newItem11->setCheckState(Qt::CheckState::Checked);
        newItem11->setFlags(newItem11->flags() & (~Qt::ItemIsUserCheckable)); // disables checkbox edit from user
        newItem11->setFlags(newItem11->flags() & (~Qt::ItemIsEditable));
        newItem11->setSizeHint(QSize(20,24));
        ui->satsTableWidget->setItem(i, 10, newItem11);
        /*
        QTableWidgetItem *newItem12 = new QTableWidgetItem();
        newItem12->setCheckState(Qt::CheckState::Unchecked);
        if (newlist[i].fSmoothed) newItem12->setCheckState(Qt::CheckState::Checked);
        newItem12->setFlags(newItem12->flags() & (~Qt::ItemIsUserCheckable)); // disables checkbox edit from user
        newItem12->setFlags(newItem12->flags() & (~Qt::ItemIsEditable));
        newItem12->setSizeHint(QSize(20,24));
        ui->satsTableWidget->setItem(i, 11, newItem12);
        */
    }
    //ui->satsTableWidget->resizeColumnsToContents();
    //ui->satsTableWidget->resizeRowsToContents();
}

void GpsSatsForm::onTimeAccReceived(quint32 acc)
{
    double tAcc=acc*1e-9;
    ui->timePrecisionLabel->setText(printReadableFloat(tAcc)+"s");
    //    ui->timePrecisionLabel->setText(QString::number(acc)+" ns");
}

void GpsSatsForm::onFreqAccReceived(quint32 acc)
{
    double fAcc=acc*1e-12;
    ui->freqPrecisionLabel->setText(printReadableFloat(fAcc)+"s/s");
}

void GpsSatsForm::onIntCounterReceived(quint32 cnt)
{
    ui->intCounterLabel->setText(QString::number(cnt));
}

void GpsSatsForm::onGpsMonHWReceived(quint16 noise, quint16 agc, quint8 antStatus, quint8 antPower, quint8 jamInd, quint8 flags)
{
    ui->lnaNoiseLabel->setText(QString::number(-noise)+" dBHz");
    ui->lnaAgcLabel->setText(QString::number(agc));
    QString str;
    switch (antStatus) {
        case 0: str="init";
                ui->antStatusLabel->setStyleSheet("QLabel { background-color : yellow }");
                break;
        case 2: str="OK";
            ui->antStatusLabel->setStyleSheet("QLabel { background-color : Window }");
            break;
        case 3: str="short";
                ui->antStatusLabel->setStyleSheet("QLabel { background-color : red }");
                break;
        case 4: str="open";
                ui->antStatusLabel->setStyleSheet("QLabel { background-color : red }");
                break;
        case 1:
        default:
                str="unknown";
                ui->antStatusLabel->setStyleSheet("QLabel { background-color : yellow }");
    }
    ui->antStatusLabel->setText(str);
    switch (antPower) {
        case 0: str="off";
                break;
        case 1: str="on";
                break;
        case 2:
        default:
                str="unknown";
    }
    ui->antPowerLabel->setText(str);
    ui->jammingProgressBar->setValue(jamInd/2.55);
}

void GpsSatsForm::onGpsMonHW2Received(qint8 ofsI, quint8 magI, qint8 ofsQ, quint8 magQ, quint8 cfgSrc)
{
    const int iqPixmapSize=65;
    QPixmap iqPixmap(iqPixmapSize,iqPixmapSize);
    //    pixmap.fill(QColor("transparent"));
    iqPixmap.fill(Qt::white);
    QPainter iqPainter(&iqPixmap);
    iqPainter.setPen(QPen(Qt::black));
    iqPainter.drawLine(QPoint(iqPixmapSize/2,0),QPoint(iqPixmapSize/2,iqPixmapSize));
    iqPainter.drawLine(QPoint(0,iqPixmapSize/2),QPoint(iqPixmapSize,iqPixmapSize/2));
    QColor col(Qt::blue);
    iqPainter.setPen(col);
    double x=0., y=0.;
    for (int i=0; i<iqTrack.size();i++) {
        iqPainter.drawPoint(iqTrack[i]);
    }
    x=ofsI*iqPixmapSize/(2*127)+iqPixmapSize/2.;
    y=-ofsQ*iqPixmapSize/(2*127)+iqPixmapSize/2.;
    col.setAlpha(100);
    iqPainter.setBrush(col);
    iqPainter.drawEllipse(QPointF(x,y),magI*iqPixmapSize/512.,magQ*iqPixmapSize/512.);
    ui->iqAlignmentLabel->setPixmap(iqPixmap);
    iqTrack.push_back(QPointF(x,y));
    if (iqTrack.size()>MAX_IQTRACK_BUFFER) iqTrack.pop_front();
}

void GpsSatsForm::onGpsVersionReceived(const QString &swString, const QString &hwString, const QString& protString)
{
    ui->ubxHwVersionLabel->setText(hwString);
    ui->ubxSwVersionLabel->setText(swString);
    ui->UBXprotLabel->setText(protString);
}

void GpsSatsForm::onGpsFixReceived(quint8 val)
{
    QString fixType = "N/A";
    if (val<FIX_TYPE_STRINGS.size()) fixType=FIX_TYPE_STRINGS[val];
    if (val<2)  ui->fixTypeLabel->setStyleSheet("QLabel { background-color : red }");
    else if (val==2)  ui->fixTypeLabel->setStyleSheet("QLabel { background-color : lightgreen }");
    else if (val>2)  ui->fixTypeLabel->setStyleSheet("QLabel { background-color : green }");
    else ui->fixTypeLabel->setStyleSheet("QLabel { background-color : Window }");
    ui->fixTypeLabel->setText(fixType);
}

void GpsSatsForm::onGeodeticPosReceived(GeodeticPos pos){

    QString str;
    str=printReadableFloat(pos.hAcc/1000.,2,0)+"m/"+printReadableFloat(pos.vAcc/1000.,2,0)+"m";
/*    str=QString::number((float)pos.hAcc/1000.,'f',3)+"m";
    str+="/"+QString::number((float)pos.vAcc/1000.,'f',3)+"m";
*/
    ui->xyzResLabel->setText(str);
}

void GpsSatsForm::onUiEnabledStateChange(bool connected)
{
    if (!connected) {
        QVector<GnssSatellite> emptylist;
        onSatsReceived(emptylist);
        iqTrack.clear();
        satTracks.clear();
        onGpsMonHW2Received(0,0,0,0,0);
        ui->jammingProgressBar->setValue(0);
        ui->timePrecisionLabel->setText("N/A");
        ui->freqPrecisionLabel->setText("N/A");
        ui->intCounterLabel->setText("N/A");
        ui->lnaNoiseLabel->setText("N/A");
        ui->lnaAgcLabel->setText("N/A");
        ui->antStatusLabel->setText("N/A");
        ui->antPowerLabel->setText("N/A");
        ui->fixTypeLabel->setText("N/A");
        ui->fixTypeLabel->setStyleSheet("QLabel { background-color : Window }");
        ui->xyzResLabel->setText("N/A");
        ui->ubxHwVersionLabel->setText("N/A");
        ui->ubxSwVersionLabel->setText("N/A");
        ui->UBXprotLabel->setText("N/A");
        ui->ubxUptimeLabel->setText("N/A");
        //ui->satsTableWidget->setRowCount(0);
    }
    ui->jammingProgressBar->setEnabled(connected);
    iqTrack.clear();
    this->setEnabled(connected);

}

void GpsSatsForm::onUbxUptimeReceived(quint32 val)
{
    ui->ubxUptimeLabel->setText(" "+QString::number(val/3600.,'f',2)+" h ");
}
