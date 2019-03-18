#ifndef CUSTOMPLOT_H
#define CUSTOMPLOT_H
#include <qwt_plot.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_curve.h>
#include <qwt_series_data.h>
//#include <qwt_scale_engine.h>
//#include <qwt_date_scale_draw.h>
//#include <qwt_date_scale_engine.h>

class CustomPlot : public QwtPlot
{
public:
    CustomPlot(QWidget *parent = 0) : QwtPlot(parent){ initialize();}
    CustomPlot(const QwtText &title, QWidget *parent = 0) : QwtPlot(title, parent){ initialize();}
    ~CustomPlot();
    void initialize();
    QwtPlotGrid *grid = nullptr;
    
	QwtPlotCurve& curve(const QString& curveName);
	void addCurve(const QString& name, const QColor& curveColor=Qt::blue);
	
	void setStatusEnabled(bool status);

    // for other plots: subclass "CustomPlot" and put all specific functions (like below) to the new class


    const QString title = "Plot";
	static QwtPlotCurve INVALID_CURVE;
private:
	QMap<QString, QwtPlotCurve*> fCurveMap;
	
};

#endif // CUSTOMPLOT_H
