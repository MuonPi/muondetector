#include <qwt.h>
#include <qwt_legend.h>
#include <qwt_scale_engine.h>
#include <QMenu>
#include <QFileDialog>
#include <numeric>

#include <custom_plot_widget.h>

#define LOG_BASELINE 0.1

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

       this->setContextMenuPolicy(Qt::CustomContextMenu);
       connect(this,SIGNAL(customContextMenuRequested(const QPoint &  )),this,SLOT(popUpMenu(const QPoint &)));

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

void CustomPlot::popUpMenu(const QPoint & pos)
{
    QMenu contextMenu(tr("Context menu"), this);

    QAction action1("&Log Y", this);
    action1.setCheckable(true);
    action1.setChecked(getLogY());
    connect(&action1, &QAction::toggled, this, [this](bool checked){ this->setLogY(checked); this->replot(); } );
    contextMenu.addAction(&action1);
/*
    contextMenu.addSeparator();
    QAction action2("&Clear", this);
//    connect(&action2, &QAction::triggered, this, &CustomHistogram::clear );
    connect(&action2, &QAction::triggered, this,  [this](bool checked){ this->clear(); this->replot(); });
    contextMenu.addAction(&action2);

    QAction action3("&Export", this);
    connect(&action3, &QAction::triggered, this, &CustomHistogram::exportToFile );
    contextMenu.addAction(&action3);
*/
    contextMenu.exec(mapToGlobal(pos));
//    contextMenu.popup(mapToGlobal(pos));
}

void CustomPlot::rescale() {
    bool succ=false;
    double ymin=1e30;
    double ymax=-1e30;
    double xmin=1e30;
    double xmax=-1e30;

    for (auto it=fCurveMap.begin(); it!=fCurveMap.end(); ++it) {
        if ((*it)->dataSize()) {
            if (xmin>(*it)->minXValue()) xmin=(*it)->minXValue();
            if (xmax<(*it)->maxXValue()) xmax=(*it)->maxXValue();
            if (ymin>(*it)->minYValue()) ymin=(*it)->minYValue();
            if (ymax<(*it)->maxYValue()) ymax=(*it)->maxYValue();
            succ=true;
        }
    }

    if (fLogY) {
        if (this->axisInterval(QwtPlot::yLeft).minValue()<=0.) {
        }
        if (this->axisAutoScale(QwtPlot::yLeft)) {
        } else {

        }
    } else {

    }

}

void CustomPlot::setLogY(bool logscale){
    if (logscale) {
        if (this->axisInterval(QwtPlot::yLeft).minValue()<=0.) {
            this->setAxisScale(QwtPlot::yLeft, LOG_BASELINE, axisInterval(QwtPlot::yLeft).maxValue());
        }
#if	QWT_VERSION > 0x060100
        setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine());
#else
        setAxisScaleEngine(QwtPlot::yLeft, new QwtLog10ScaleEngine());
#endif
        setAxisAutoScale(QwtPlot::yLeft,true);
        //fBarChart->setBaseline(1e-12);
        fLogY=true;
    } else {
        setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine());
        setAxisAutoScale(QwtPlot::yLeft,true);
        //fBarChart->setBaseline(0);
        fLogY=false;
    }
    emit scalingChanged();
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

