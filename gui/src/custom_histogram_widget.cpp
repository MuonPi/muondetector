#include "custom_histogram_widget.h"

#include "histogram_series_data.h"

#include <QApplication>
#include <QColor>
#include <QEvent>
#include <QFileDialog>
#include <QFont>
#include <QLabel>
#include <QMenu>
#include <QResizeEvent>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <qbrush.h>
#include <qpen.h>
#include <qtextstream.h>
#include <qwt.h>
#include <qwt_legend.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_renderer.h>
#include <qwt_scale_engine.h>
#include <qwt_text.h>

import muondetector.histogram;

typedef std::numeric_limits<double> dbl;

CustomHistogram::~CustomHistogram() {
    if (grid != nullptr) {
        delete grid;
        grid = nullptr;
    }
    if (fBarChart != nullptr) {
        delete fBarChart;
        fBarChart = nullptr;
    }
    if (fFitCurve != nullptr) {
        delete fFitCurve;
        fFitCurve = nullptr;
    }
    if (fFitOverlayLabel != nullptr) {
        delete fFitOverlayLabel;
        fFitOverlayLabel = nullptr;
    }
}

void CustomHistogram::initialize() {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setAutoFillBackground(true);
    QwtPlotCanvas* canvas = dynamic_cast<QwtPlotCanvas*>(this->canvas());
    canvas->setFrameStyle(QFrame::NoFrame);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QApplication::palette().color(QPalette::Base));
    setPalette(pal);
    setCanvasBackground(QApplication::palette().color(QPalette::Base));
    setAutoReplot(true);
    enableAxis(QwtPlot::yLeft, true);
    enableAxis(QwtPlot::yRight, false);
    setAxisAutoScale(QwtPlot::xBottom, true);
    setAxisAutoScale(QwtPlot::yLeft, true);
    grid = new QwtPlotGrid();
    const QPen grayPen(Qt::gray);
    grid->setPen(grayPen);
    grid->attach(this);
    fBarChart = new QwtPlotHistogram(title);

    fBarChart->setBrush(QBrush(Qt::darkBlue, Qt::SolidPattern));
    fBarChart->attach(this);
    fFitCurve = new QwtPlotCurve("Fit");
    fFitCurve->setPen(QPen(QColor(190, 40, 40), 2));
    fFitCurve->setRenderHint(QwtPlotCurve::RenderAntialiased, true);
    fFitCurve->attach(this);

    fFitOverlayLabel = new QLabel(canvas);
    fFitOverlayLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    fFitOverlayLabel->setWordWrap(false);
    fFitOverlayLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    fFitOverlayLabel->hide();
    styleFitOverlay();

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this,
            SLOT(popUpMenu(const QPoint&)));

    replot();
    show();
}

void CustomHistogram::changeEvent(QEvent* e) {
    if (e->type() == QEvent::PaletteChange) {
        // update canvas background to appropriate theme
        QPalette pal = palette();
        pal.setColor(QPalette::Window, QApplication::palette().color(QPalette::Base));
        setPalette(pal);
        setCanvasBackground(QApplication::palette().color(QPalette::Base));
        styleFitOverlay();
        replot();
    }
    QwtPlot::changeEvent(e);
}

void CustomHistogram::resizeEvent(QResizeEvent* event) {
    QwtPlot::resizeEvent(event);
    positionFitOverlay();
}

void CustomHistogram::setData(const Histogram& hist) {
    fName = hist.getName();
    fUnit = hist.getUnit();
    fNrBins = hist.getNrBins();
    fMin = hist.getMin();
    fMax = hist.getMax();
    fUnderflow = hist.getUnderflow();
    fOverflow = hist.getOverflow();
    fHistogramMap.clear();
    for (int i = 0; i < fNrBins; i++)
        fHistogramMap[i] = hist.getBinContent(i);
    update();
}

auto CustomHistogram::formatFitValue(double value, int precision) const -> QString {
    if (!std::isfinite(value)) {
        return "N/A";
    }
    return QString::number(value, 'g', precision);
}

auto CustomHistogram::shouldShowFit() const -> bool {
    const QString name = QString::fromStdString(fName).toLower();
    const QString plotTitle = title.toLower();

    return !(name.contains("phase") || plotTitle.contains("phase") || name == "pulseheight" ||
             name.contains("pulse height") || plotTitle.contains("pulse height") ||
             name == "ubxeventlength");
}

void CustomHistogram::popUpMenu(const QPoint& pos) {
    QMenu contextMenu(tr("Context menu"), this);

    QAction action1("&Log Y", this);
    action1.setCheckable(true);
    action1.setChecked(getLogY());
    connect(&action1, &QAction::toggled, this, [this](bool checked) {
        this->setLogY(checked);
        this->update();
    });
    contextMenu.addAction(&action1);
    contextMenu.addSeparator();
    QAction action2("&Clear", this);
    connect(&action2, &QAction::triggered, this, [this](bool /*checked*/) {
        this->clear();
        this->update();
    });
    contextMenu.addAction(&action2);

    QAction action3("&Export", this);
    connect(&action3, &QAction::triggered, this, &CustomHistogram::exportToFile);
    contextMenu.addAction(&action3);

    contextMenu.exec(mapToGlobal(pos));
}

void CustomHistogram::exportToFile() {
    QPixmap qPix = grab();
    if (qPix.isNull()) {
        qDebug("Failed to capture the plot for saving");
        return;
    }
    QString types("JPEG file (*.jpeg);;" // Set up the possible graphics formats
                  "Portable Network Graphics file (*.png);;"
                  "Bitmap file (*.bmp);;"
                  "Portable Document Format (*.pdf);;"
                  "Scalable Vector Graphics Format (*.svg);;"
                  "ASCII raw data (*.txt)");
    QString filter; // Type of filter
    QString jpegExt = ".jpeg", pngExt = ".png", tifExt = ".tif", bmpExt = ".bmp",
            tif2Ext = "tiff"; // Suffix for the files
    QString pdfExt = ".pdf", svgExt = ".svg";
    QString txtExt = ".txt";
    QString suggestedName = "";
    QString fn =
        QFileDialog::getSaveFileName(this, tr("Export Histogram"), suggestedName, types, &filter);

    if (!fn.isEmpty()) {            // If filename is not null
        if (fn.contains(jpegExt)) { // Remove file extension if already there
            fn.remove(jpegExt);
        } else if (fn.contains(pngExt)) {
            fn.remove(pngExt);
        } else if (fn.contains(bmpExt)) {
            fn.remove(bmpExt);
        } else if (fn.contains(pdfExt)) {
            fn.remove(pdfExt);
        } else if (fn.contains(svgExt)) {
            fn.remove(svgExt);
        } else if (fn.contains(txtExt)) {
            fn.remove(txtExt);
        }

        if (filter.contains(jpegExt)) { // OR, Test to see if jpeg and save
            fn += jpegExt;
            qPix.save(fn, "JPEG");
        } else if (filter.contains(pngExt)) { // OR, Test to see if png and save
            fn += pngExt;
            qPix.save(fn, "PNG");
        } else if (filter.contains(bmpExt)) { // OR, Test to see if bmp and save
            fn += bmpExt;
            qPix.save(fn, "BMP");
        } else if (filter.contains(pdfExt)) {
            fn += pdfExt;
            QwtPlotRenderer renderer(this);
            renderer.renderDocument(this, fn, "pdf", QSizeF(297 / 2, 210 / 2), 72);
        } else if (filter.contains(svgExt)) {
            fn += svgExt;
            QwtPlotRenderer renderer(this);
            renderer.renderDocument(this, fn, "svg", QSizeF(297 / 2, 210 / 2), 72);
        }
        if (filter.contains(txtExt)) {
            fn += txtExt;
            // export histo in asci raw data format
            QFile file(fn);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return;
            QTextStream out(&file);

            for (int i = 0; i < fNrBins; i++) {
                out << QString::number(bin2Value(i), 'g', dbl::max_digits10) << "  "
                    << fHistogramMap[i] << "\n";
            }
        }
    }
}

void CustomHistogram::update() {
    if (!isEnabled())
        return;
    if (fHistogramMap.empty() || fNrBins <= 1) {
        fBarChart->detach();
        fFitCurve->detach();
        hideFitOverlay();
        QwtPlot::replot();
        return;
    }
    QVector<QwtIntervalSample> intervals;
    double rangeX = fMax - fMin;
    double xBinSize = rangeX / (fNrBins - 1);
    double max = 0;
    for (int i = 0; i < fNrBins; i++) {
        if (fHistogramMap[i] > max)
            max = fHistogramMap[i];
        double xval = bin2Value(i);
        QwtIntervalSample interval(fHistogramMap[i] + 1e-12, xval - xBinSize / 2.,
                                   xval + xBinSize / 2.);
        intervals.push_back(interval);
    }
    if (intervals.size() && fBarChart != nullptr)
        // fBarChart->setSamples(intervals.toList());
        fBarChart->setData(new HistogramSeriesData(intervals));
    fBarChart->attach(this);
    const QString fitText = fShowFit ? buildFitOverlay(max) : QString{};
    if (fitText.isEmpty()) {
        fFitCurve->detach();
        hideFitOverlay();
    } else {
        showFitOverlay(fitText);
        fFitCurve->attach(this);
    }
    if (fLogY) {
        setAxisScale(QwtPlot::yLeft, 0.1, 1.5 * max);
    }
    replot();
}

auto CustomHistogram::buildFitOverlay(double yMax) -> QString {
    if (fFitCurve == nullptr || fFitOverlayLabel == nullptr || yMax <= 0. || fNrBins <= 1) {
        return {};
    }
    if (!shouldShowFit()) {
        return {};
    }

    const double entries = getEntries();
    if (entries <= 1.) {
        return {};
    }

    const double rangeX = fMax - fMin;
    const double binWidth = rangeX / (fNrBins - 1);
    if (rangeX <= 0. || binWidth <= 0.) {
        return {};
    }

    const double mean = getMean();
    const double sigma = getRMS();
    QVector<QPointF> points;
    constexpr int nPoints = 300;

    if (fName.find("Interval") != std::string::npos) {
        int peakBin{-1};
        double peakContent{0.};
        for (const auto& [bin, content] : fHistogramMap) {
            if (content <= 0.) {
                continue;
            }
            const double x = bin2Value(bin);
            if (x < 0. || !std::isfinite(x)) {
                continue;
            }
            if (content > peakContent) {
                peakBin = bin;
                peakContent = content;
            }
        }

        if (peakBin < 0 || peakContent <= 0.) {
            return {};
        }

        const double x0 = bin2Value(peakBin);
        QVector<QPointF> fitBins;
        double weightedOffset{0.};
        double tailEntries{0.};
        for (const auto& [bin, content] : fHistogramMap) {
            if (bin < peakBin || content <= 0.) {
                continue;
            }
            const double x = bin2Value(bin);
            if (!std::isfinite(x) || x < x0) {
                continue;
            }
            fitBins.push_back(QPointF(x, content));
            weightedOffset += content * (x - x0);
            tailEntries += content;
        }

        if (fitBins.size() < 3 || tailEntries <= 0.) {
            return {};
        }

        double amplitude = peakContent;
        double tau = weightedOffset / tailEntries;
        if (!std::isfinite(tau) || tau <= 0.) {
            tau = binWidth;
        }
        tau = std::max(tau, 0.1 * binWidth);

        double chi2{0.};
        for (int iteration = 0; iteration < 60; ++iteration) {
            double hAA{0.};
            double hAT{0.};
            double hTT{0.};
            double bA{0.};
            double bT{0.};

            for (const QPointF& sample : fitBins) {
                const double t = sample.x() - x0;
                const double observed = sample.y();
                const double exponential = std::exp(-t / tau);
                const double expected = amplitude * exponential;
                const double residual = observed - expected;
                const double weight = 1. / std::max(observed, 1.);
                const double dAmplitude = exponential;
                const double dTau = amplitude * exponential * t / (tau * tau);

                hAA += weight * dAmplitude * dAmplitude;
                hAT += weight * dAmplitude * dTau;
                hTT += weight * dTau * dTau;
                bA += weight * dAmplitude * residual;
                bT += weight * dTau * residual;
            }

            const double det = hAA * hTT - hAT * hAT;
            if (std::abs(det) <= std::numeric_limits<double>::epsilon()) {
                break;
            }

            const double deltaAmplitude = (bA * hTT - bT * hAT) / det;
            const double deltaTau = (hAA * bT - hAT * bA) / det;
            if (!std::isfinite(deltaAmplitude) || !std::isfinite(deltaTau)) {
                break;
            }

            amplitude = std::max(amplitude + deltaAmplitude, std::numeric_limits<double>::min());
            tau = std::max(tau + deltaTau, 0.1 * binWidth);

            const double relativeAmplitudeStep = std::abs(deltaAmplitude) / std::max(amplitude, 1.);
            const double relativeTauStep = std::abs(deltaTau) / std::max(tau, binWidth);
            if (relativeAmplitudeStep < 1e-6 && relativeTauStep < 1e-6) {
                break;
            }
        }

        for (const QPointF& sample : fitBins) {
            const double t = sample.x() - x0;
            const double observed = sample.y();
            const double expected = amplitude * std::exp(-t / tau);
            const double residual = observed - expected;
            chi2 += residual * residual / std::max(observed, 1.);
        }

        const double fitMax = std::max(x0, fMax);
        for (int i = 0; i < nPoints; ++i) {
            const double x = x0 + (fitMax - x0) * static_cast<double>(i) / (nPoints - 1);
            const double y = amplitude * std::exp(-(x - x0) / tau);
            if (std::isfinite(y) && y > 0.) {
                points.push_back(QPointF(x, y));
            }
        }
        fFitCurve->setSamples(points);

        double intervalSeconds = tau;
        if (fUnit == "ms") {
            intervalSeconds *= 1e-3;
        } else if (fUnit == "us") {
            intervalSeconds *= 1e-6;
        } else if (fUnit == "ns") {
            intervalSeconds *= 1e-9;
        }

        const int ndf = fitBins.size() - 2;
        QString text = "Poisson process\nfit tau=" + formatFitValue(tau) +
                       QString::fromStdString(fUnit) + "\nA=" + formatFitValue(amplitude) +
                       "\nx0=" + formatFitValue(x0) + QString::fromStdString(fUnit) +
                       "\nmean=" + formatFitValue(mean) + QString::fromStdString(fUnit) +
                       "\noverflow=" + formatFitValue(fOverflow);
        if (intervalSeconds > 0.) {
            text += "\nrate=" + QString::number(1. / intervalSeconds, 'g', 4) + " Hz";
        }
        if (ndf > 0) {
            text += "\nchi2/ndf=" + QString::number(chi2 / ndf, 'g', 3);
        }
        if (sigma > 0. && std::isfinite(sigma)) {
            text += "\nCV=" + QString::number(sigma / mean, 'g', 3);
        }
        return text;
    }

    if (sigma <= 0. || !std::isfinite(mean) || !std::isfinite(sigma)) {
        return {};
    }

    constexpr double pi = 3.14159265358979323846;
    const double norm = entries * binWidth / (sigma * std::sqrt(2. * pi));
    for (int i = 0; i < nPoints; ++i) {
        const double x = fMin + rangeX * static_cast<double>(i) / (nPoints - 1);
        const double z = (x - mean) / sigma;
        points.push_back(QPointF(x, norm * std::exp(-0.5 * z * z)));
    }
    fFitCurve->setSamples(points);

    QString text = "Gaussian\nmu=" + formatFitValue(mean) + QString::fromStdString(fUnit) +
                   "\nsigma=" + formatFitValue(sigma) + QString::fromStdString(fUnit) +
                   "\nFWHM=" + formatFitValue(2.354820045 * sigma) + QString::fromStdString(fUnit) +
                   "\noverflow=" + formatFitValue(fOverflow);
    if (std::abs(mean) > std::numeric_limits<double>::epsilon()) {
        text += "\nrel=" + QString::number(std::abs(sigma / mean) * 100., 'g', 3) + "%";
    }
    return text;
}

void CustomHistogram::styleFitOverlay() {
    if (fFitOverlayLabel == nullptr) {
        return;
    }

    QFont font = QApplication::font();
    if (font.pointSize() > 0) {
        font.setPointSize(font.pointSize() + 1);
    }
    fFitOverlayLabel->setFont(font);

    const QColor base = QApplication::palette().color(QPalette::Base);
    if (base.lightness() > 128) {
        fFitOverlayLabel->setStyleSheet(
            "QLabel { color: black; border: 1px solid black; border-radius: 2px; "
            "background-color: rgba(255, 255, 255, 220); padding: 6px 8px; }");
    } else {
        fFitOverlayLabel->setStyleSheet(
            "QLabel { color: white; border: 1px solid white; border-radius: 2px; "
            "background-color: rgba(0, 0, 0, 0); padding: 6px 8px; }");
    }
}

void CustomHistogram::positionFitOverlay() {
    if (fFitOverlayLabel == nullptr || !fFitOverlayLabel->isVisible()) {
        return;
    }

    QWidget* plotCanvas = canvas();
    if (plotCanvas == nullptr) {
        return;
    }

    const int margin = 10;
    fFitOverlayLabel->setMaximumWidth(std::max(120, plotCanvas->width() / 2));
    fFitOverlayLabel->adjustSize();
    const int x = std::max(margin, plotCanvas->width() - fFitOverlayLabel->width() - margin);
    fFitOverlayLabel->move(x, margin);
    fFitOverlayLabel->raise();
}

void CustomHistogram::showFitOverlay(const QString& text) {
    if (fFitOverlayLabel == nullptr) {
        return;
    }

    styleFitOverlay();
    fFitOverlayLabel->setText(text);
    fFitOverlayLabel->show();
    positionFitOverlay();
}

void CustomHistogram::hideFitOverlay() {
    if (fFitOverlayLabel != nullptr) {
        fFitOverlayLabel->hide();
    }
}

void CustomHistogram::clear() {
    Histogram::clear();
    emit histogramCleared(QString::fromStdString(fName));
    fName = "defaultHisto";
    update();
}

void CustomHistogram::setEnabled(bool status) {
    if (status == true) {
        const QPen blackPen(Qt::black);
        grid->setPen(blackPen);
        fBarChart->attach(this);
        setTitle(title);
    } else {
        const QPen grayPen(Qt::gray);
        grid->setPen(grayPen);
        fBarChart->detach();
        fFitCurve->detach();
        hideFitOverlay();
        setTitle("");
    }
    fEnabled = status;
    update();
}

void CustomHistogram::setLogY(bool logscale) {
    if (logscale) {
#if QWT_VERSION > 0x060100
        setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine());
#else
        setAxisScaleEngine(QwtPlot::yLeft, new QwtLog10ScaleEngine());
#endif
        setAxisAutoScale(QwtPlot::yLeft, false);
        fBarChart->setBaseline(1e-12);
        fLogY = true;
    } else {
        setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
        setAxisAutoScale(QwtPlot::yLeft, true);
        fBarChart->setBaseline(0);
        fLogY = false;
    }
}

void CustomHistogram::setShowFit(bool show) {
    fShowFit = show;
    if (!fShowFit) {
        fFitCurve->detach();
        hideFitOverlay();
    }
    update();
}

void CustomHistogram::setXMin(double val) {
    fMin = val;
    rescalePlot();
}

void CustomHistogram::setXMax(double val) {
    fMax = val;
    rescalePlot();
}

void CustomHistogram::rescalePlot() {
    double margin = 0.05 * (fMax - fMin);
    setAxisScale(QwtPlot::xBottom, fMin - margin, fMax + margin);
}
