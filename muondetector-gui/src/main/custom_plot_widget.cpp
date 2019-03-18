#include <custom_plot_widget.h>
#include <qwt_legend.h>
#include <qwt.h>

QwtPlotCurve CustomPlot::INVALID_CURVE;

CustomPlot::~CustomPlot(){
    for (auto it=fCurveMap.begin(); it!=fCurveMap.end(); ++it) {
		if (*it!=nullptr) delete *it;
	}
	if (grid!=nullptr) { delete grid; grid=nullptr;}
}

void CustomPlot::initialize(){
       setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
       setStyleSheet("background-color: white; border: 0px;");
       setAutoReplot(false);
       enableAxis(QwtPlot::yLeft,true);
       //enableAxis(QwtPlot::yRight,true);
       setAxisAutoScale(QwtPlot::xBottom,true);
       setAxisAutoScale(QwtPlot::yRight,true);

       grid = new QwtPlotGrid();
       const QPen blackPen(Qt::black);
       grid->setPen(blackPen);
       grid->attach(this);

/*       xorCurve = new QwtPlotCurve();
       xorCurve->setYAxis(QwtPlot::yRight);
       xorCurve->setRenderHint(QwtPlotCurve::RenderAntialiased, true);
       //xorCurve->setStyle(QwtPlotCurve::Steps);
       QColor xorCurveColor = Qt::darkGreen;
       xorCurveColor.setAlphaF(0.3);
       const QPen greenPen(xorCurveColor);
       xorCurve->setPen(greenPen);
       xorCurve->setBrush(xorCurveColor);
       xorCurve->attach(this);
*/
       show();
}

QwtPlotCurve& CustomPlot::curve(const QString& curveName)
{
	auto it = fCurveMap.find(curveName);
	if (it!=fCurveMap.end()) return **it;
	return INVALID_CURVE;
}


void CustomPlot::addCurve(const QString& name, const QColor& curveColor)
{
	QwtPlotCurve *curve = new QwtPlotCurve();
	curve->setYAxis(QwtPlot::yLeft);
	curve->setRenderHint(QwtPlotCurve::RenderAntialiased, true);
	//curve->setStyle(QwtPlotCurve::Steps);
	//QColor curveColor = Qt::darkGreen;
	//curveColor.setAlphaF(0.3);
	const QPen pen(curveColor);
	curve->setPen(pen);
	//curve->setBrush(curveColor);
	fCurveMap[name]=curve;
	fCurveMap[name]->attach(this);
}


void CustomPlot::setStatusEnabled(bool status){
    if (status==true){
//        curve->attach(this);
        setTitle(title);
        replot();
    }else{
//        curve->detach();
        setTitle("");
        replot();
    }
}

