#ifndef CUSTOMHISTOGRAM_H
#define CUSTOMHISTOGRAM_H
#include <QVector>
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_series_data.h>
#include <qwt_plot_histogram.h>
#include <histogram.h>

class QwtPlotHistogram;
class Histogram;

class CustomHistogram : public QwtPlot, public Histogram
{
	Q_OBJECT
signals:
    void histogramCleared(QString histogramName);
public:
    CustomHistogram(QWidget *parent = 0) : QwtPlot(parent){ initialize();}
    CustomHistogram(const QwtText &title, QWidget *parent = 0) : QwtPlot(title, parent){ initialize();}
    ~CustomHistogram();
    void initialize();
    QwtPlotGrid *grid = nullptr;
    QString title = "Histogram";
    //virtual void replot();

public slots:
    void update();
	void clear();
	void setStatusEnabled(bool status);
	void setXMin(double val);
	void setXMax(double val);
	bool getLogX() const { return fLogX; }
    void setLogY(bool);
	bool getLogY() const { return fLogY; }
	void rescalePlot();

	QwtPlotHistogram* getHistogramPlot() { return fBarChart; }
    
    void setData(const Histogram& hist);

private slots:
    void popUpMenu(const QPoint &pos);
    void exportToFile();

private:
	QwtPlotHistogram* fBarChart = nullptr;
	bool fLogY=false;
	bool fLogX=false;
};

#endif // CUSTOMHISTOGRAM_H
