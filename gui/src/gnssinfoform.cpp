#include "gnssinfoform.h"
#include "ui_gnssinfoform.h"
#include <QPainter>
#include <QPixmap>
#include <cmath>

constexpr int MAX_IQTRACK_BUFFER { 250 };

// helper function to format human readable numbers with common suffixes (k(ilo), M(ega), m(illi) etc.)
QString printReadableFloat(double value, int prec = 2, int lowOrderInhibit = -12, int highOrderInhibit = 9)
{
    QString str = "";
    QString suffix = "";
    double order = std::log10(std::fabs(value));
    if (order >= lowOrderInhibit && order <= highOrderInhibit) {
        if (order >= 9) {
            value *= 1e-9;
            suffix = "G";
        } else if (order >= 6) {
            value *= 1e-6;
            suffix = "M";
        } else if (order >= 3) {
            value *= 1e-3;
            suffix = "k";
        } else if (order >= 0) {
            suffix = "";
        } else if (order >= -3) {
            value *= 1000.;
            suffix = "m";
        } else if (order >= -6) {
            value *= 1e6;
            suffix = "u";
        } else if (order >= -9) {
            value *= 1e9;
            suffix = "n";
        } else if (order >= -12) {
            value *= 1e12;
            suffix = "p";
        }
    }
    char fmtChar = 'f';
    double newOrder = std::log10(std::fabs(value));
    if (fabs(newOrder) >= 3.) {
        fmtChar = 'g';
    } else {
        prec = prec - (int)newOrder - 1;
    }
    if (prec < 0)
        prec = 0;
    return QString::number(value, fmtChar, prec) + " " + suffix;
}

GnssInfoForm::GnssInfoForm(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::GnssInfoForm)
{
    ui->setupUi(this);
}

GnssInfoForm::~GnssInfoForm()
{
    delete ui;
}

void GnssInfoForm::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::PaletteChange) {
        // update canvas background to appropriate theme
        replotIqWidget();
    }
    QWidget::changeEvent(e);
}

void GnssInfoForm::onSatsReceived(const QVector<GnssSatellite>& satlist)
{
    ui->gnssPosWidget->onSatsReceived(satlist);

    int nrGoodSats { 0 };
    ui->satsTableWidget->clearContents();
    ui->satsTableWidget->setRowCount(0);
    for (const auto& current_sat : satlist) {
        if (current_sat.fCnr > 0) {
            nrGoodSats++;
        } else if (ui->visibleSatsCheckBox->isChecked()) {
            continue;
        }
        ui->satsTableWidget->insertRow(ui->satsTableWidget->rowCount());
        QTableWidgetItem* newItem1 = new QTableWidgetItem;
        newItem1->setData(Qt::DisplayRole, current_sat.fSatId);
        newItem1->setSizeHint(QSize(30, 24));
        newItem1->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(ui->satsTableWidget->rowCount() - 1, 0, newItem1);

        QTableWidgetItem* newItem2 = new QTableWidgetItem(QString::fromLocal8Bit(Gnss::Id::name[std::clamp(static_cast<int>(current_sat.fGnssId), Gnss::Id::first, Gnss::Id::last)]));
        newItem2->setSizeHint(QSize(50, 24));
        newItem2->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(newItem1->row(), 1, newItem2);

        QTableWidgetItem* newItem3 = new QTableWidgetItem;
        newItem3->setData(Qt::DisplayRole, current_sat.fCnr);
        newItem3->setSizeHint(QSize(50, 24));
        newItem3->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(newItem1->row(), 2, newItem3);

        QTableWidgetItem* newItem4 = new QTableWidgetItem;
        newItem4->setData(Qt::DisplayRole, current_sat.fAzim);
        newItem4->setSizeHint(QSize(60, 24));
        newItem4->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(newItem1->row(), 3, newItem4);

        QTableWidgetItem* newItem5 = new QTableWidgetItem;
        newItem5->setData(Qt::DisplayRole, current_sat.fElev);
        newItem5->setSizeHint(QSize(60, 24));
        newItem5->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(newItem1->row(), 4, newItem5);

        QTableWidgetItem* newItem6 = new QTableWidgetItem;
        newItem6->setData(Qt::DisplayRole, current_sat.fPrRes);
        newItem6->setSizeHint(QSize(60, 24));
        newItem6->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(newItem1->row(), 5, newItem6);

        QTableWidgetItem* newItem7 = new QTableWidgetItem;
        newItem7->setData(Qt::DisplayRole, current_sat.fQuality);
        QColor color { Qt::green };
        double transp { std::clamp(0.166 * (current_sat.fQuality - 1), 0., 1.) };
        color.setAlphaF(transp);
        newItem7->setBackground(color);
        newItem7->setSizeHint(QSize(25, 24));
        newItem7->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(newItem1->row(), 6, newItem7);

        QString str { "n/a" };
        if (current_sat.fHealth < GNSS_HEALTH_STRINGS.size())
            str = GNSS_HEALTH_STRINGS[current_sat.fHealth];
        if (current_sat.fHealth == 0) {
            color = Qt::lightGray;
        } else if (current_sat.fHealth == 1) {
            color = Qt::green;
        } else if (current_sat.fHealth >= 2) {
            color = Qt::red;
        }
        QTableWidgetItem* newItem8 = new QTableWidgetItem(str);
        newItem8->setSizeHint(QSize(25, 24));
        newItem8->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(newItem1->row(), 7, newItem8);

        int orbSrc { std::clamp(current_sat.fOrbitSource, 0, 7) };
        QTableWidgetItem* newItem9 = new QTableWidgetItem(GNSS_ORBIT_SRC_STRING[std::clamp(orbSrc, 0, GNSS_ORBIT_SRC_STRING.size())]);
        newItem9->setSizeHint(QSize(40, 24));
        newItem9->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(newItem1->row(), 8, newItem9);

        QTableWidgetItem* newItem10 = new QTableWidgetItem();
        newItem10->setCheckState({ (current_sat.fUsed) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked });
        newItem10->setFlags(newItem10->flags() & (~Qt::ItemIsUserCheckable)); // disables checkbox edit from user
        newItem10->setFlags(newItem10->flags() & (~Qt::ItemIsEditable));
        newItem10->setSizeHint(QSize(20, 24));
        newItem10->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(newItem1->row(), 9, newItem10);

        QTableWidgetItem* newItem11 = new QTableWidgetItem();
        newItem11->setCheckState({ (current_sat.fDiffCorr) ? Qt::CheckState::Checked : Qt::CheckState::Unchecked });
        newItem11->setFlags(newItem11->flags() & (~Qt::ItemIsUserCheckable)); // disables checkbox edit from user
        newItem11->setFlags(newItem11->flags() & (~Qt::ItemIsEditable));
        newItem11->setSizeHint(QSize(20, 24));
        newItem11->setTextAlignment(Qt::AlignHCenter);
        ui->satsTableWidget->setItem(newItem1->row(), 10, newItem11);
    }
    ui->nrSatsLabel->setText(QString::number(nrGoodSats) + "/" + QString::number(satlist.size()));
}

void GnssInfoForm::onTimeAccReceived(quint32 acc)
{
    ui->timePrecisionLabel->setText(printReadableFloat(acc * 1e-9) + "s");
}

void GnssInfoForm::onFreqAccReceived(quint32 acc)
{
    ui->freqPrecisionLabel->setText(printReadableFloat(acc * 1e-12) + "s/s");
}

void GnssInfoForm::onIntCounterReceived(quint32 cnt)
{
    ui->intCounterLabel->setText(QString::number(cnt));
}

void GnssInfoForm::onGpsMonHWReceived(const GnssMonHwStruct& hwstruct)
{
    ui->lnaNoiseLabel->setText(QString::number(-hwstruct.noise) + " dBHz");
    ui->lnaAgcLabel->setText(QString::number(hwstruct.agc));
    QString str { "n/a" };
    if (hwstruct.antStatus < GNSS_ANT_STATUS_STRINGS.size())
        str = GNSS_ANT_STATUS_STRINGS[hwstruct.antStatus];
    switch (hwstruct.antStatus) {
    case 0:
        ui->antStatusLabel->setStyleSheet("QLabel { background-color : yellow }");
        break;
    case 2:
        ui->antStatusLabel->setStyleSheet("QLabel { background-color : Window }");
        break;
    case 3:
        ui->antStatusLabel->setStyleSheet("QLabel { background-color : red }");
        break;
    case 4:
        ui->antStatusLabel->setStyleSheet("QLabel { background-color : red }");
        break;
    case 1:
    default:
        ui->antStatusLabel->setStyleSheet("QLabel { background-color : yellow }");
    }
    ui->antStatusLabel->setText(str);
    switch (hwstruct.antPower) {
    case 0:
        str = "off";
        break;
    case 1:
        str = "on";
        break;
    case 2:
    default:
        str = "unknown";
    }
    ui->antPowerLabel->setText(str);
    ui->jammingProgressBar->setValue(hwstruct.jamInd / 2.55);
}

void GnssInfoForm::replotIqWidget()
{
    constexpr int iqPixmapSize { 65 };
    QPixmap iqPixmap(iqPixmapSize, iqPixmapSize);
    iqPixmap.fill(QApplication::palette().color(QPalette::Base));
    QPainter iqPainter(&iqPixmap);
    iqPainter.setPen(QPen(QApplication::palette().color(QPalette::WindowText)));
    iqPainter.drawLine(QPoint(iqPixmapSize / 2, 0), QPoint(iqPixmapSize / 2, iqPixmapSize));
    iqPainter.drawLine(QPoint(0, iqPixmapSize / 2), QPoint(iqPixmapSize, iqPixmapSize / 2));
    QColor col { Qt::blue };
    iqPainter.setPen(col);
    for (const auto& iqPoint : iqTrack) {
        iqPainter.drawPoint(iqPoint);
    }
    double x { fIqData.ofsI * iqPixmapSize / (2 * 127) + iqPixmapSize / 2. };
    double y { -fIqData.ofsQ * iqPixmapSize / (2 * 127) + iqPixmapSize / 2. };
    col.setAlpha(100);
    iqPainter.setBrush(col);
    iqPainter.drawEllipse(QPointF(x, y), fIqData.magI * iqPixmapSize / 512., fIqData.magQ * iqPixmapSize / 512.);
    ui->iqAlignmentLabel->setPixmap(iqPixmap);
    iqTrack.push_back(QPointF(x, y));
    if (iqTrack.size() > MAX_IQTRACK_BUFFER)
        iqTrack.pop_front();
}

void GnssInfoForm::onGpsMonHW2Received(const GnssMonHw2Struct& hw2struct)
{
    fIqData = hw2struct;
    replotIqWidget();
}

void GnssInfoForm::onGpsVersionReceived(const QString& swString, const QString& hwString, const QString& protString)
{
    ui->ubxHwVersionLabel->setText(hwString);
    ui->ubxSwVersionLabel->setText(swString);
    ui->UBXprotLabel->setText(protString);
}

void GnssInfoForm::onGpsFixReceived(quint8 val)
{
    QString fixType { Gnss::FixType::name[Gnss::FixType::None] };
    if (val < Gnss::FixType::count)
        fixType = QString::fromLocal8Bit(Gnss::FixType::name[val]);
    if (val < 2)
        ui->fixTypeLabel->setStyleSheet("QLabel { background-color : red }");
    else if (val == 2)
        ui->fixTypeLabel->setStyleSheet("QLabel { background-color : lightgreen }");
    else if (val > 2)
        ui->fixTypeLabel->setStyleSheet("QLabel { background-color : green }");
    else
        ui->fixTypeLabel->setStyleSheet("QLabel { background-color : Window }");
    ui->fixTypeLabel->setText(fixType);
}

void GnssInfoForm::onGeodeticPosReceived(const GeodeticPos& pos)
{
    QString str { printReadableFloat(pos.hAcc / 1000., 2, 4) + "m / " + printReadableFloat(pos.vAcc / 1000., 2, 4) + "m" };
    ui->xyzResLabel->setText(str);
}

void GnssInfoForm::onUiEnabledStateChange(bool connected)
{
    if (!connected) {
        QVector<GnssSatellite> emptylist;
        onSatsReceived(emptylist);
        iqTrack.clear();
        onGpsMonHW2Received(GnssMonHw2Struct());
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
    }
    ui->jammingProgressBar->setEnabled(connected);
    iqTrack.clear();
    this->setEnabled(connected);
}

void GnssInfoForm::onUbxUptimeReceived(quint32 val)
{
    ui->ubxUptimeLabel->setText(" " + QString::number(val / 3600., 'f', 2) + " h ");
}
