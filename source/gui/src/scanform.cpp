#include "scanform.h"
#include "ui_scanform.h"
#include <muondetector_structs.h>
#include <QVector>
#include <QPointF>
#include <qwt_symbol.h>

ScanForm::ScanForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ScanForm)
{
    ui->setupUi(this);
	ui->scanParComboBox->clear();
	foreach (QString item, SP_NAMES) {
        ui->scanParComboBox->addItem(item);
    }
	ui->observableComboBox->clear();
	foreach (QString item, OP_NAMES) {
        ui->observableComboBox->addItem(item);
    }
	ui->minRangeLineEdit->setValidator(new QDoubleValidator(.001, 3.3, 4, ui->minRangeLineEdit));
	ui->maxRangeLineEdit->setValidator(new QDoubleValidator(.001, 3.3, 4, ui->maxRangeLineEdit));
	ui->stepSizeLineEdit->setValidator(new QDoubleValidator(0., 1.0, 5, ui->stepSizeLineEdit));

    ui->scanPlot->setTitle("Parameter Scan");
    ui->scanPlot->setAxisTitle(QwtPlot::xBottom,"scanpar");
    ui->scanPlot->setAxisTitle(QwtPlot::yLeft,"observable");

    ui->scanPlot->addCurve("curve1", Qt::blue);
    ui->scanPlot->curve("curve1").setStyle(QwtPlotCurve::NoCurve);
    QwtSymbol *sym=new QwtSymbol(QwtSymbol::Rect, QBrush(Qt::blue, Qt::SolidPattern),QPen(Qt::black),QSize(5,5));
    ui->scanPlot->curve("curve1").setSymbol(sym);
    ui->scanPlot->setAxisAutoScale(QwtPlot::xBottom, true);
    ui->scanPlot->setAxisAutoScale(QwtPlot::yLeft, true);
    ui->scanPlot->replot();

}

ScanForm::~ScanForm()
{
    delete ui;
}

void ScanForm::onTimeMarkReceived(const UbxTimeMarkStruct &tm)
{
	if (!tm.risingValid) {
		qDebug()<<"received invalid timemark";
		return;
	}
	if (active && OP_NAMES[obsPar]=="UBXRATE") {
		if (waitForFirst) {
			currentCounts=0;
			currentTimeInterval=0.;
			waitForFirst=false;
		} else {
			currentCounts+=(uint16_t)(tm.evtCounter-lastTM.evtCounter);
			double interval=(tm.rising.tv_nsec-lastTM.rising.tv_nsec)*1e-9;
			interval+=tm.rising.tv_sec-lastTM.rising.tv_sec;
			currentTimeInterval+=interval;
			if (currentTimeInterval>maxMeasurementTimeInterval) scanParIteration();
		}
	}
    lastTM=tm;
}

void ScanForm::scanParIteration() {
	if (!active) return;
	if (OP_NAMES[obsPar]=="UBXRATE") {
		double rate=currentCounts/currentTimeInterval;
		scanData[currentScanPar]=rate;
		updateScanPlot();
	}
	currentScanPar+=stepSize;
	adjustScanPar(SP_NAMES[scanPar], currentScanPar);
	ui->scanProgressBar->setValue((currentScanPar-minRange)/stepSize);
	if (currentScanPar>maxRange) {
		// measurement finished
		finishScan();
		for (auto it=scanData.constBegin(); it!=scanData.constEnd(); ++it) {
			qDebug()<<it.key()<<"  "<<it.value();
		}
		return;
	}
	waitForFirst=true;
}

void ScanForm::adjustScanPar(QString scanParName, double value) {
	if (scanParName=="THR1") {
		emit setDacVoltage(0, value);
	} else if (scanParName=="THR2") {
		emit setDacVoltage(1, value);
	}
}

void ScanForm::on_scanStartPushButton_clicked()
{
	if (active) {
		finishScan();
		return;
	}
	bool ok=false;
	minRange = ui->minRangeLineEdit->text().toDouble(&ok);
	if (!ok) return;
	maxRange = ui->maxRangeLineEdit->text().toDouble(&ok);
	if (!ok) return;
	stepSize = ui->stepSizeLineEdit->text().toDouble(&ok);
	if (!ok) return;
	scanPar=ui->scanParComboBox->currentIndex();
	if (SP_NAMES[scanPar]=="VOID") return;
	obsPar=ui->observableComboBox->currentIndex();
	if (OP_NAMES[obsPar]=="VOID") return;
	
	maxMeasurementTimeInterval=ui->timeIntervalSpinBox->value();
	
	if (OP_NAMES[obsPar]=="UBXRATE") emit gpioInhibitChanged(true);
	// values seem to be valid
    // start the scan
	ui->scanStartPushButton->setText(tr("Stop Scan"));
	currentScanPar=minRange;
	adjustScanPar(SP_NAMES[scanPar], currentScanPar);
	active=true;
	waitForFirst=true;
	scanData.clear();
	ui->scanProgressBar->setRange(0, (maxRange-minRange)/stepSize+0.5);
	ui->scanProgressBar->reset();
}

void ScanForm::finishScan() {
	ui->scanStartPushButton->setText(tr("Start Scan"));
	active=false;
	ui->scanProgressBar->reset();
	emit gpioInhibitChanged(false);
	updateScanPlot();
}

void ScanForm::updateScanPlot() {
	QVector<QPointF> vec;
	for (auto it=scanData.constBegin(); it!=scanData.constEnd(); ++it) {
        QPointF p1;
        if (!plotDifferential) {
			p1.rx()=it.key();
			p1.ry()=it.value();
		} else {
			if (it==--scanData.constEnd()) continue;
			p1.rx()=it.key();
			auto it2 = it;
			++it2;
			p1.ry()=it.value()-it2.value();
		}
        vec.push_back(p1);
	}

    ui->scanPlot->curve("curve1").setSamples(vec);
    ui->scanPlot->replot();
}

void ScanForm::on_plotDifferentialCheckBox_toggled(bool checked)
{
    plotDifferential=checked;
	updateScanPlot();
}
