#include "logplotswidget.h"
#include "ui_logplotswidget.h"
#include <QDateTime>
#include <qwt_symbol.h>

LogPlotsWidget::LogPlotsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LogPlotsWidget)
{
    ui->setupUi(this);
    //ui->logPlot->setTitle("temp");
    ui->logPlot->setAxisTitle(QwtPlot::xBottom,"time");
    //ui->logPlot->setAxisTitle(QwtPlot::yLeft,"temp / °C");
    connect(ui->logPlot, &CustomPlot::scalingChanged,this, &LogPlotsWidget::onScalingChanged);
    ui->logPlot->addCurve("curve1", Qt::blue);
    ui->logPlot->curve("curve1").setStyle(QwtPlotCurve::NoCurve);
    QwtSymbol *sym=new QwtSymbol(QwtSymbol::Rect, QBrush(Qt::blue, Qt::SolidPattern),QPen(Qt::black),QSize(5,5));
    ui->logPlot->curve("curve1").setSymbol(sym);
    ui->logPlot->setAxisAutoScale(QwtPlot::xBottom, true);
    ui->logPlot->setAxisAutoScale(QwtPlot::yLeft, true);
    ui->logPlot->replot();

}

LogPlotsWidget::~LogPlotsWidget()
{
    delete ui;
}

void LogPlotsWidget::onTemperatureReceived(float temp)
{
    auto it = fLogMap.find("Temperature");
    if (it==fLogMap.end()) {
        fLogMap["Temperature"].setName("Temperature");
        fLogMap["Temperature"].setUnit("°C");
    }
    QPointF p;
    p.setX(QDateTime::currentMSecsSinceEpoch()/1000ULL);
    p.setY(temp);
    fLogMap["Temperature"].push_back(p);
    updateLogTable();
}

void LogPlotsWidget::onTimeAccReceived(quint32 acc)
{
    auto it = fLogMap.find("Time Accuracy");
    if (it==fLogMap.end()) {
        fLogMap["Time Accuracy"].setName("Time Accuracy");
        fLogMap["Time Accuracy"].setUnit("ns");
    }
    QPointF p;
    p.setX(QDateTime::currentMSecsSinceEpoch()/1000ULL);
    p.setY(acc);
    fLogMap["Time Accuracy"].push_back(p);
    updateLogTable();
}

void LogPlotsWidget::onBiasVoltageCalculated(float ubias)
{
    const QString name = "SiPM Bias Voltage";
    auto it = fLogMap.find(name);
    if (it==fLogMap.end()) {
        fLogMap[name].setName(name);
        fLogMap[name].setUnit("V");
    }
    QPointF p;
    p.setX(QDateTime::currentMSecsSinceEpoch()/1000ULL);
    p.setY(ubias);
    fLogMap[name].push_back(p);
    updateLogTable();
}

void LogPlotsWidget::onBiasCurrentCalculated(float ibias)
{
    const QString name = "SiPM Bias Current";
    auto it = fLogMap.find(name);
    if (it==fLogMap.end()) {
        fLogMap[name].setName(name);
        fLogMap[name].setUnit("uA");
    }
    QPointF p;
    p.setX(QDateTime::currentMSecsSinceEpoch()/1000ULL);
    p.setY(ibias);
    fLogMap[name].push_back(p);
    updateLogTable();
}


void LogPlotsWidget::updateLogTable()
{
    ui->nrLogsLabel->setText(QString::number(fLogMap.size()));
    ui->tableWidget->setRowCount(fLogMap.size());
    int i=0;
    for (auto it=fLogMap.begin(); it!=fLogMap.end(); it++)
    {
        QTableWidgetItem *newItem1 = new QTableWidgetItem(it.key());
        newItem1->setSizeHint(QSize(120,24));
        ui->tableWidget->setItem(i, 0, newItem1);
        QTableWidgetItem *newItem2 = new QTableWidgetItem(QString::number(it.value().data().size()));
        newItem2->setSizeHint(QSize(100,24));
        ui->tableWidget->setItem(i, 1, newItem2);
        i++;
    }

    if (fCurrentLog.size()) {
        for (int i=0; i<ui->tableWidget->rowCount(); i++) {
            if (ui->tableWidget->item(i,0)->text()==fCurrentLog) {
                on_tableWidget_cellClicked(i,0);
            }
        }
    }

}

void LogPlotsWidget::on_tableWidget_cellClicked(int row, int column)
{
    QString name = ui->tableWidget->item(row,0)->text();
//    ui->nrHistosLabel->setText(name);
    auto it = fLogMap.find(name);
    if (it!=fLogMap.end()) {
        fCurrentLog=name;
        ui->logPlot->setTitle(name);
        //ui->histoWidget->setData(*it);
        ui->logPlot->curve("curve1").setSamples(it->data());
        ui->logPlot->setAxisTitle(QwtPlot::xBottom, "time");
        ui->logPlot->setAxisTitle(QwtPlot::yLeft, it->getUnit());
        ui->logNameLabel->setText(it->getName());
        ui->logPlot->replot();
        onScalingChanged();
    }
}

void LogPlotsWidget::onUiEnabledStateChange(bool connected)
{
    if (!connected) {
        ui->tableWidget->setRowCount(0);
        ui->logPlot->curve("curve1").hide();
        ui->logNameLabel->setText("N/A");
        ui->nrLogsLabel->setText(QString::number(0));
        fLogMap.clear();
        fCurrentLog="";
        ui->logPlot->replot();
    } else {
        ui->logPlot->curve("curve1").show();
    }
    this->setEnabled(connected);
    ui->logPlot->setEnabled(connected);
}

void LogPlotsWidget::onScalingChanged()
{
    ui->xMinLineEdit->setText(QString::number(ui->logPlot->axisInterval(QwtPlot::xBottom).minValue()));
    ui->xMaxLineEdit->setText(QString::number(ui->logPlot->axisInterval(QwtPlot::xBottom).maxValue()));
    ui->yMinLineEdit->setText(QString::number(ui->logPlot->axisInterval(QwtPlot::yLeft).minValue()));
    ui->yMaxLineEdit->setText(QString::number(ui->logPlot->axisInterval(QwtPlot::yLeft).maxValue()));
}

void LogPlotsWidget::on_linesCheckBox_clicked()
{
   if (ui->linesCheckBox->isChecked()) ui->logPlot->curve("curve1").setStyle(QwtPlotCurve::Lines);
   else ui->logPlot->curve("curve1").setStyle(QwtPlotCurve::NoCurve);
   ui->logPlot->replot();
}

void LogPlotsWidget::on_pointSizeSpinBox_valueChanged(int arg1)
{
    QwtSymbol *sym=new QwtSymbol(QwtSymbol::Rect, QBrush(Qt::blue, Qt::SolidPattern),QPen(Qt::black),QSize(arg1,arg1));
    ui->logPlot->curve("curve1").setSymbol(sym);
    ui->logPlot->replot();
}
