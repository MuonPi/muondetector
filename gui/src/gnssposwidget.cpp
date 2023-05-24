#include "gnssposwidget.h"
#include "ui_gnssposwidget.h"
#include <QDebug>
#include <QFileDialog>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QResizeEvent>
#include <QTextStream>
#include <QTransform>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <ublox_structs.h>
#define _USE_MATH_DEFINES

namespace Detail {
double constexpr sqrtNewtonRaphson(double x, double curr, double prev)
{
    return curr == prev
        ? curr
        : sqrtNewtonRaphson(x, 0.5 * (curr + x / curr), curr);
}
double constexpr sqrt(double x)
{
    return x >= 0 && x < std::numeric_limits<double>::infinity()
        ? Detail::sqrtNewtonRaphson(x, x, 0)
        : std::numeric_limits<double>::quiet_NaN();
}
}
constexpr double pi() { return M_PI; }
constexpr double sqrt2() { return Detail::sqrt(2.); }

constexpr int MAX_SAT_TRACK_ENTRIES { 1000 };

static const QVector<QColor> GNSS_COLORS = { Qt::darkGreen, Qt::darkYellow, Qt::blue, Qt::magenta, Qt::gray, Qt::cyan, Qt::red, Qt::black };

int GnssPosWidget::alphaFromCnr(int cnr, int range)
{
    if (range <= 0)
        return 0;
    int alpha = cnr * 255 / range;
    return std::clamp(alpha, 0, 255);
}

GnssPosWidget::GnssPosWidget(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::GnssPosWidget)
{
    ui->setupUi(this);
    ui->satLabel->setText(QString::number(ui->satLabel->width()) + "x" + QString::number(ui->satLabel->height()));
    connect(ui->receivedSatsCheckBox, &QCheckBox::toggled, this, &GnssPosWidget::replot);
    connect(ui->satLabelsCheckBox, &QCheckBox::toggled, this, &GnssPosWidget::replot);
    connect(ui->tracksCheckBox, &QCheckBox::toggled, this, &GnssPosWidget::replot);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(popUpMenu(const QPoint&)));
    connect(ui->cartPolarCheckBox, &QCheckBox::toggled, this, [this](bool /*checked*/) { resizeEvent(nullptr); });
    connect(ui->cnrRangeSpinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, [this](int) { replot(); });
}

GnssPosWidget::~GnssPosWidget()
{
    delete ui;
}

void GnssPosWidget::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::PaletteChange) {
        // update canvas background to appropriate theme
        replot();
    }
    QWidget::changeEvent(e);
}

void GnssPosWidget::resizeEvent(QResizeEvent* /*event*/)
{
    int w { ui->hostWidget->width() };
    int h { ui->hostWidget->height() };
    if (ui->cartPolarCheckBox->isChecked()) {
        if (h < w)
            w = h;
        else
            h = w;
    } else {
        // no constraints
    }
    w -= 10;
    h -= 10;
    if (w < 10)
        w = 10;
    if (h < 10)
        h = 10;
    ui->satLabel->resize(w, h);

    replot();
}

QPolygonF GnssPosWidget::getPolarUnitPolygon(const QPointF& pos, int controlPoints)
{
    const double step { 1. / (controlPoints + 1) };
    QPolygonF path {};
    QPointF p(pos + QPointF(-0.5, -0.5));
    for (int side = 0; side < 4; side++) {
        for (int i = 0; i < controlPoints + 1; i++) {
            QPointF incrPoint {};
            switch (side) {
            case 0:
                incrPoint = QPointF(step, 0.);
                break;
            case 1:
                incrPoint = QPointF(0., step);
                break;
            case 2:
                incrPoint = QPointF(-step, 0.);
                break;
            case 3:
                incrPoint = QPointF(0., -step);
                break;
            default:
                break;
            }
            p += incrPoint;
            path.append(p);
        }
    }
    return path;
}

QPolygonF GnssPosWidget::getCartPolygonUnity(const QPointF& polarPos)
{
    QPolygonF polarRect { getPolarUnitPolygon(polarPos, DEFAULT_CONTROL_POINTS + qAbs(polarPos.y() / 10)) };
    QPolygonF cartPolygon {};
    for (QPointF p : polarRect) {
        cartPolygon.append(polar2cartUnity(p));
    }
    return cartPolygon;
}

QPointF GnssPosWidget::polar2cartUnity(const QPointF& pol)
{
    if (pol.y() >= 90.)
        return QPointF(0., 0.);
    double magn = (90. - pol.y()) / 180.;
    double xpos = magn * sin(pi() * pol.x() / 180.);
    double ypos = -magn * cos(pi() * pol.x() / 180.);
    return QPointF(xpos, ypos);
}

void GnssPosWidget::drawPolarPixMap(QPixmap& pm)
{
    QVector<GnssSatellite> newlist {};

    const int satPosPixmapSize { pm.width() };
    const QPointF originOffset(satPosPixmapSize / 2., satPosPixmapSize / 2.);

    pm.fill(QApplication::palette().color(QPalette::Base));
    QPainter satPosPainter(&pm);
    satPosPainter.setPen(QPen(QApplication::palette().color(QPalette::WindowText)));
    satPosPainter.drawEllipse(QPoint(satPosPixmapSize / 2, satPosPixmapSize / 2), satPosPixmapSize / 6, satPosPixmapSize / 6);
    satPosPainter.drawEllipse(QPoint(satPosPixmapSize / 2, satPosPixmapSize / 2), satPosPixmapSize / 3, satPosPixmapSize / 3);
    satPosPainter.drawEllipse(QPoint(satPosPixmapSize / 2, satPosPixmapSize / 2), satPosPixmapSize / 2, satPosPixmapSize / 2);
    satPosPainter.drawLine(QPoint(satPosPixmapSize / 2, 0), QPoint(satPosPixmapSize / 2, satPosPixmapSize));
    satPosPainter.drawLine(QPoint(0, satPosPixmapSize / 2), QPoint(satPosPixmapSize, satPosPixmapSize / 2));
    satPosPainter.drawText(satPosPixmapSize / 2 + 2, 3, 18, 18, Qt::AlignHCenter, "N");
    satPosPainter.drawText(satPosPixmapSize / 2 + 2, satPosPixmapSize - 19, 18, 18, Qt::AlignHCenter, "S");
    satPosPainter.drawText(4, satPosPixmapSize / 2 - 19, 18, 18, Qt::AlignHCenter, "W");
    satPosPainter.drawText(satPosPixmapSize - 19, satPosPixmapSize / 2 - 19, 18, 18, Qt::AlignHCenter, "E");

    QFont font = satPosPainter.font();
    font.setPointSize(font.pointSize() - 2);
    satPosPainter.setFont(font);
    satPosPainter.drawText(satPosPixmapSize / 2 - 14, satPosPixmapSize - 12, 18, 18, Qt::AlignHCenter, "0°");
    satPosPainter.drawText(satPosPixmapSize / 2 - 16, satPosPixmapSize * 5 / 6 - 12, 18, 18, Qt::AlignHCenter, "30°");
    satPosPainter.drawText(satPosPixmapSize / 2 - 16, satPosPixmapSize * 2 / 3 - 12, 18, 18, Qt::AlignHCenter, "60°");

    // set up coordinate transformation
    QTransform trafo {};
    trafo.translate(originOffset.x(), originOffset.y());
    trafo.scale(satPosPixmapSize, satPosPixmapSize);
    satPosPainter.setTransform(trafo);

    // draw the sat tracks first
    if (ui->tracksCheckBox->isChecked()) {
        for (const auto& pointMap : satTracks) {
            for (const auto& pointVector : pointMap) {
                if (pointVector.isEmpty())
                    continue;
                const double mean_cnr {
                    std::accumulate(pointVector.constBegin(), pointVector.constEnd(), 0, [](int sum, const SatHistoryPoint& satpoint) {
                        return sum + satpoint.cnr;
                    })
                    / static_cast<double>(pointVector.size())
                };
                const auto satPoint { pointVector.front() };
                QColor satColor { GNSS_COLORS.at(std::clamp(static_cast<int>(satPoint.gnssId), 0, GNSS_COLORS.size() - 1)) };
                satColor.setAlpha(alphaFromCnr(mean_cnr, ui->cnrRangeSpinBox->value()));
                satPosPainter.setPen(QColor("transparent"));
                satPosPainter.setBrush(satColor);
                QPainterPath path;
                path.addPolygon(getCartPolygonUnity(satPoint.posPolar));
                satPosPainter.drawPolygon(path.toFillPolygon(QTransform()));
            }
        }
    }

    // draw the current sat positions now
    for (const auto& currentSat : fCurrentSatlist) {
        if (ui->receivedSatsCheckBox->isChecked()) {
            if (currentSat.Cnr > 0)
                newlist.push_back(currentSat);
        } else {
            newlist.push_back(currentSat);
        }
        if (currentSat.Elev <= 90. && currentSat.Elev >= -90.) {
            if (ui->receivedSatsCheckBox->isChecked() && currentSat.Cnr == 0)
                continue;
            QPointF currPos(currentSat.Azim, currentSat.Elev);
            QPointF currPoint { polar2cartUnity(currPos) };
            QColor satColor { GNSS_COLORS.at(std::clamp(static_cast<int>(currentSat.GnssId), 0, GNSS_COLORS.size() - 1)) };
            satPosPainter.setPen(satColor);
            QColor fillColor { satColor };
            int alpha = alphaFromCnr(currentSat.Cnr, ui->cnrRangeSpinBox->value());
            fillColor.setAlpha(alpha);
            int satId { currentSat.GnssId * 1000 + currentSat.SatId };
            if (currentSat.Cnr > 0) {
                SatHistoryPoint p {};
                p.posCart = currPoint;
                p.color = fillColor;
                p.posPolar = QPoint(currPos.x(), currPos.y());
                p.gnssId = currentSat.GnssId;
                p.satId = currentSat.SatId;
                p.cnr = currentSat.Cnr;
                p.time = QDateTime::currentDateTimeUtc();
                satTracks[satId][p.posPolar].push_back(p);
                if (satTracks[satId][p.posPolar].size() > MAX_SAT_TRACK_ENTRIES)
                    satTracks[satId][p.posPolar].pop_front();
            }
            satPosPainter.setPen(satColor);
            satPosPainter.setBrush(fillColor);

            // somehow, drawEllipse doesn't work with a painter when a transform is set before (bug?)
            // so reset the transform of the painter and do the scaling and shifting manually
            satPosPainter.setTransform(QTransform());
            currPoint *= satPosPixmapSize;
            currPoint += originOffset;

            int satsize { ui->satSizeSpinBox->value() };
            satPosPainter.drawEllipse(currPoint, satsize / 2., satsize / 2.);
            if (currentSat.Used) {
                satPosPainter.setPen(QApplication::palette().color(QPalette::WindowText));
                satPosPainter.drawEllipse(currPoint, 1.2 * satsize / 2., 1.2 * satsize / 2.);
                satPosPainter.setPen(satColor);
            }
            currPoint.rx() += satsize / 2 + 4;
            if (ui->satLabelsCheckBox->isChecked())
                satPosPainter.drawText(currPoint, QString::number(currentSat.SatId));
        }
    }
}

void GnssPosWidget::drawCartesianPixMap(QPixmap& pm)
{
    QVector<GnssSatellite> newlist {};

    const QPointF originOffset(0., pm.height() * 0.9);

    pm.fill(QApplication::palette().color(QPalette::Base));
    QPainter satPosPainter(&pm);
    satPosPainter.setPen(QPen(QApplication::palette().color(QPalette::WindowText)));
    satPosPainter.drawLine(originOffset, originOffset + QPointF(pm.width(), 0));
    satPosPainter.drawLine(0.33 * originOffset, 0.33 * originOffset + QPointF(pm.width(), 0));
    satPosPainter.drawLine(0.67 * originOffset, 0.67 * originOffset + QPointF(pm.width(), 0));
    satPosPainter.drawLine(QPoint(pm.width() / 2, 0), originOffset + QPointF(pm.width() / 2, 0));

    satPosPainter.drawText(pm.width() / 2 - 3, originOffset.y() + 6, 18, 18, Qt::AlignHCenter, "S");
    satPosPainter.drawText(2, originOffset.y() + 6, 18, 18, Qt::AlignHCenter, "N");
    satPosPainter.drawText(pm.width() - 18, originOffset.y() + 6, 18, 18, Qt::AlignHCenter, "N");
    satPosPainter.drawText(pm.width() / 4 - 3, originOffset.y() + 6, 18, 18, Qt::AlignHCenter, "E");
    satPosPainter.drawText(3 * pm.width() / 4 - 3, originOffset.y() + 6, 18, 18, Qt::AlignHCenter, "W");

    QFont font = satPosPainter.font();
    font.setPointSize(font.pointSize() - 2);
    satPosPainter.setFont(font);
    satPosPainter.drawText(pm.width() / 2, originOffset.y() - 10, 18, 18, Qt::AlignHCenter, "0°");
    satPosPainter.drawText(pm.width() / 2, 0.33 * originOffset.y() + 3, 18, 18, Qt::AlignHCenter, "60°");
    satPosPainter.drawText(pm.width() / 2, 0.67 * originOffset.y() + 3, 18, 18, Qt::AlignHCenter, "30°");

    // draw the sat tracks first
    if (ui->tracksCheckBox->isChecked()) {
        for (const auto& pointMap : satTracks) {
            for (const auto& pointVector : pointMap) {
                if (pointVector.isEmpty())
                    continue;
                const double mean_cnr {
                    std::accumulate(pointVector.constBegin(), pointVector.constEnd(), 0, [](int sum, const SatHistoryPoint& satpoint) {
                        return sum + satpoint.cnr;
                    })
                    / static_cast<double>(pointVector.size())
                };
                const auto satPoint { pointVector.front() };
                QColor satColor { GNSS_COLORS.at(std::clamp(static_cast<int>(satPoint.gnssId), 0, GNSS_COLORS.size() - 1)) };
                satColor.setAlpha(alphaFromCnr(mean_cnr, ui->cnrRangeSpinBox->value()));
                satPosPainter.setPen(QColor("transparent"));
                satPosPainter.setBrush(satColor);

                QPainterPath path;
                path.addPolygon(getPolarUnitPolygon(satPoint.posPolar));

                QTransform trafo;
                trafo.translate(0., 9 * pm.height() / 10.);
                trafo.scale(pm.width() / 360., -9. * pm.height() / 900.);
                satPosPainter.drawPolygon(path.toFillPolygon(trafo));
            }
        }
    }

    // draw the current sat positions now
    for (const auto& currentSat : fCurrentSatlist) {
        if (ui->receivedSatsCheckBox->isChecked()) {
            if (currentSat.Cnr > 0)
                newlist.push_back(currentSat);
        } else {
            newlist.push_back(currentSat);
        }
        if (currentSat.Elev <= 90. && currentSat.Elev >= -90.) {
            if (ui->receivedSatsCheckBox->isChecked() && currentSat.Cnr == 0)
                continue;
            QPointF currPos(currentSat.Azim, currentSat.Elev);
            QPointF currPoint = polar2cartUnity(currPos);
            QColor satColor { GNSS_COLORS.at(std::clamp(static_cast<int>(currentSat.GnssId), 0, GNSS_COLORS.size() - 1)) };
            satPosPainter.setPen(satColor);
            QColor fillColor { satColor };
            int alpha { alphaFromCnr(currentSat.Cnr, ui->cnrRangeSpinBox->value()) };
            fillColor.setAlpha(alpha);
            int satId { currentSat.GnssId * 1000 + currentSat.SatId };

            if (currentSat.Cnr > 0) {
                SatHistoryPoint p {};
                p.posCart = currPoint;
                p.color = fillColor;
                p.posPolar = QPoint(currPos.x(), currPos.y());
                p.gnssId = currentSat.GnssId;
                p.satId = currentSat.SatId;
                p.cnr = currentSat.Cnr;
                p.time = QDateTime::currentDateTimeUtc();
                satTracks[satId][p.posPolar].push_back(p);
                if (satTracks[satId][p.posPolar].size() > MAX_SAT_TRACK_ENTRIES)
                    satTracks[satId][p.posPolar].pop_front();
            }
            satPosPainter.setPen(satColor);
            satPosPainter.setBrush(fillColor);

            currPos = QPointF(currPos.x() / 360. * pm.width(), -currPos.y() / 90. * pm.height() * 0.9 + pm.height() * 0.9);

            int satsize { ui->satSizeSpinBox->value() };
            satPosPainter.drawEllipse(currPos, satsize / 2., satsize / 2.);
            if (currentSat.Used) {
                satPosPainter.setPen(QApplication::palette().color(QPalette::WindowText));
                satPosPainter.drawEllipse(currPos, 1.2 * satsize / 2., 1.2 * satsize / 2.);
                satPosPainter.setPen(satColor);
            }
            currPos.rx() += satsize / 2 + 4;
            if (ui->satLabelsCheckBox->isChecked())
                satPosPainter.drawText(currPos, QString::number(currentSat.SatId));
        }
    }
}

void GnssPosWidget::replot()
{
    const int w { ui->satLabel->width() };
    const int h { ui->satLabel->height() };

    QPixmap satPosPixmap(w, h);
    if (ui->cartPolarCheckBox->isChecked()) {
        drawPolarPixMap(satPosPixmap);
    } else {
        drawCartesianPixMap(satPosPixmap);
    }

    ui->satLabel->setPixmap(satPosPixmap);
}

void GnssPosWidget::onSatsReceived(const QVector<GnssSatellite>& satlist)
{
    fCurrentSatlist = satlist;
    replot();
}

void GnssPosWidget::on_satSizeSpinBox_valueChanged(int /*arg1*/)
{
    replot();
}

void GnssPosWidget::onUiEnabledStateChange(bool connected)
{
    if (!connected) {
        fCurrentSatlist.clear();
    }
    this->setEnabled(connected);
}

void GnssPosWidget::popUpMenu(const QPoint& pos)
{
    QMenu contextMenu(tr("Context menu"), this);

    QAction action2("&Clear Tracks", this);
    connect(&action2, &QAction::triggered, this, [this](bool /*checked*/) { satTracks.clear() ; this->replot(); });
    contextMenu.addAction(&action2);

    contextMenu.addSeparator();

    QAction action3("&Export", this);
    connect(&action3, &QAction::triggered, this, &GnssPosWidget::exportToFile);
    contextMenu.addAction(&action3);

    contextMenu.exec(mapToGlobal(pos));
}

void GnssPosWidget::exportToFile()
{
    constexpr int pixmapSize { 512 };
    QPixmap satPosPixmap {};
    if (ui->cartPolarCheckBox->isChecked()) {
        satPosPixmap = QPixmap(pixmapSize, pixmapSize);
        drawPolarPixMap(satPosPixmap);
    } else {
        satPosPixmap = QPixmap(sqrt2() * pixmapSize, pixmapSize);
        drawCartesianPixMap(satPosPixmap);
    }

    QString types("JPEG file (*.jpeg);;" // Set up the possible graphics formats
                  "Portable Network Graphics file (*.png);;"
                  "Bitmap file (*.bmp);;"
                  "ASCII raw data (*.txt)");
    QString filter {}; // Type of filter
    QString jpegExt = ".jpeg", pngExt = ".png", tifExt = ".tif", bmpExt = ".bmp", tif2Ext = "tiff"; // Suffix for the files
    QString txtExt = ".txt";
    QString suggestedName { "" };
    QString fn = QFileDialog::getSaveFileName(this, tr("Export Pixmap"),
        suggestedName, types, &filter);

    if (!fn.isEmpty()) { // If filename is not null
        if (fn.contains(jpegExt)) { // Remove file extension if already there
            fn.remove(jpegExt);
        } else if (fn.contains(pngExt)) {
            fn.remove(pngExt);
        } else if (fn.contains(bmpExt)) {
            fn.remove(bmpExt);
        } else if (fn.contains(txtExt)) {
            fn.remove(txtExt);
        }

        if (filter.contains(jpegExt)) { // OR, Test to see if jpeg and save
            fn += jpegExt;
            satPosPixmap.save(fn, "JPEG");
        } else if (filter.contains(pngExt)) { // OR, Test to see if png and save
            fn += pngExt;
            satPosPixmap.save(fn, "PNG");
        } else if (filter.contains(bmpExt)) { // OR, Test to see if bmp and save
            fn += bmpExt;
            satPosPixmap.save(fn, "BMP");
        } else if (filter.contains(txtExt)) {
            fn += txtExt;
            // export sat history in asci raw data format
            QFile file(fn);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return;
            QTextStream out(&file);
            out << "# GNSS sat history from  xxx to yyy\n";
            out << "# datetime marks the entrance time into space point (az,el)\n";
            out << "# <ISO8601-datetime> <gnss-id> <gnss-id-string> <sat-id> <azimuth> <elevation> <cnr>\n";

            for (const auto& pointMap : satTracks) {
                for (const auto& pointVector : pointMap) {
                    if (pointVector.isEmpty())
                        continue;
                    const double mean_cnr {
                        std::accumulate(pointVector.constBegin(), pointVector.constEnd(), 0, [](int sum, const SatHistoryPoint& satpoint) {
                            return sum + satpoint.cnr;
                        })
                        / static_cast<double>(pointVector.size())
                    };

                    SatHistoryPoint p { pointVector.front() };
                    out << p.time.toString(Qt::ISODate) << " "
                        << p.gnssId << " "
                        << QString::fromLocal8Bit(Gnss::Id::name[std::clamp(static_cast<int>(p.gnssId), static_cast<int>(Gnss::Id::first), static_cast<int>(Gnss::Id::last))]) << p.satId << " "
                        << p.posPolar.x() << " "
                        << p.posPolar.y() << " "
                        << mean_cnr << "\n";
                }
            }
        }
    }
}
