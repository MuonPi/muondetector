#ifndef CUSTOMHISTOGRAM_H
#define CUSTOMHISTOGRAM_H

#include "histogram.h"

#include <QVector>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_histogram.h>
#include <qwt_series_data.h>

class QLabel;
class QwtPlotHistogram;

class CustomHistogram : public QwtPlot, public Histogram {
    Q_OBJECT
  signals:
    void histogramCleared(QString histogramName);

  public:
    CustomHistogram(QWidget* parent = 0) : QwtPlot(parent) { initialize(); }
    CustomHistogram(const QwtText& title_l, QWidget* parent = 0) : QwtPlot(title_l, parent) {
        initialize();
    }
    ~CustomHistogram();
    void initialize();
    bool enabled() const { return fEnabled; }
    QwtPlotGrid* grid = nullptr;
    QString title = "Histogram";

  public slots:
    void update();
    void clear();
    void setEnabled(bool status);
    void setXMin(double val);
    void setXMax(double val);
    bool getLogX() const { return fLogX; }
    void setLogY(bool);
    bool getLogY() const { return fLogY; }
    void setShowFit(bool show);
    bool getShowFit() const { return fShowFit; }
    void rescalePlot();
    void changeEvent(QEvent* e) override;

    QwtPlotHistogram* getHistogramPlot() { return fBarChart; }

    void setData(const Histogram& hist);

  private slots:
    void popUpMenu(const QPoint& pos);
    void exportToFile();

  private:
    QwtPlotHistogram* fBarChart = nullptr;
    QwtPlotCurve* fFitCurve = nullptr;
    QLabel* fFitOverlayLabel = nullptr;
    auto buildFitOverlay(double yMax) -> QString;
    auto formatFitValue(double value, int precision = 4) const -> QString;
    auto shouldShowFit() const -> bool;
    void showFitOverlay(const QString& text);
    void hideFitOverlay();
    void positionFitOverlay();
    void styleFitOverlay();
    bool fLogY = false;
    bool fLogX = false;
    bool fShowFit = true;
    bool fEnabled{false};

  protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif // CUSTOMHISTOGRAM_H
