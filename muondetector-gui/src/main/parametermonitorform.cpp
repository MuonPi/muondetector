#include <qwt_symbol.h>
#include "parametermonitorform.h"
#include "ui_parametermonitorform.h"
#include <muondetector_structs.h>

ParameterMonitorForm::ParameterMonitorForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ParameterMonitorForm)
{
    ui->setupUi(this);
    ui->adcTracePlot->setTitle("ADC trace");
    ui->adcTracePlot->setAxisTitle(QwtPlot::xBottom,"sample nr. since trigger");
    ui->adcTracePlot->setAxisTitle(QwtPlot::yLeft,"U / V");

    ui->adcTracePlot->addCurve("curve1", Qt::blue);
    ui->adcTracePlot->curve("curve1").setStyle(QwtPlotCurve::NoCurve);
    QwtSymbol *sym=new QwtSymbol(QwtSymbol::Rect, QBrush(Qt::blue, Qt::SolidPattern),QPen(Qt::black),QSize(5,5));
    ui->adcTracePlot->curve("curve1").setSymbol(sym);
    ui->adcTracePlot->setAxisAutoScale(QwtPlot::xBottom, false);
    ui->adcTracePlot->setAxisAutoScale(QwtPlot::yLeft, false);
    ui->adcTracePlot->setAxisScale(QwtPlot::xBottom, -10., 40. );
    ui->adcTracePlot->setAxisScale(QwtPlot::yLeft, 0., 3.5 );
    ui->adcTracePlot->replot();
    foreach (GpioSignalDescriptor item, GPIO_SIGNAL_MAP) {
        if (item.direction==DIR_IN) ui->adcTriggerSelectionComboBox->addItem(item.name);
    }

}

ParameterMonitorForm::~ParameterMonitorForm()
{
    delete ui;
}

void ParameterMonitorForm::onAdcSampleReceived(uint8_t channel, float value)
{
    if (channel==0)
        ui->adcLabel1->setText(QString::number(value,'f',4));
    else if (channel==1)
        ui->adcLabel2->setText(QString::number(value,'f',4));
    else if (channel==2)
        ui->adcLabel3->setText(QString::number(value,'f',4));
    else if (channel==3)
        ui->adcLabel4->setText(QString::number(value,'f',4));
}

void ParameterMonitorForm::onDacReadbackReceived(uint8_t channel, float value)
{
    ui->dacSpinBox1->blockSignals(true);
    ui->dacSpinBox2->blockSignals(true);
    ui->dacSpinBox3->blockSignals(true);
    ui->dacSpinBox4->blockSignals(true);
    ui->dacSlider1->blockSignals(true);
    ui->dacSlider2->blockSignals(true);
    ui->dacSlider3->blockSignals(true);
    ui->dacSlider4->blockSignals(true);
    if (channel==0) {
        ui->dacSpinBox1->setValue(value);
        ui->dacSlider1->setValue(value*1000);
    } else if (channel==1) {
        ui->dacSpinBox2->setValue(value);
        ui->dacSlider2->setValue(value*1000);
    } else if (channel==2) {
        ui->dacSpinBox3->setValue(value);
        ui->dacSlider3->setValue(value*1000);
    } else if (channel==3) {
        ui->dacSpinBox4->setValue(value);
        ui->dacSlider4->setValue(value*1000);
    }
    ui->dacSpinBox1->blockSignals(false);
    ui->dacSpinBox2->blockSignals(false);
    ui->dacSpinBox3->blockSignals(false);
    ui->dacSpinBox4->blockSignals(false);
    ui->dacSlider1->blockSignals(false);
    ui->dacSlider2->blockSignals(false);
    ui->dacSlider3->blockSignals(false);
    ui->dacSlider4->blockSignals(false);
}

void ParameterMonitorForm::onInputSwitchReceived(uint8_t index)
{
    ui->timingSelectionComboBox->setCurrentIndex(index);
}

void ParameterMonitorForm::onBiasSwitchReceived(bool state)
{
    ui->biasEnCheckBox->setChecked(state);
}

void ParameterMonitorForm::onPreampSwitchReceived(uint8_t channel, bool state)
{
    if (channel==0) ui->preamp1EnCheckBox->setChecked(state);
    else if (channel==1) ui->preamp2EnCheckBox->setChecked(state);
}

void ParameterMonitorForm::onTriggerSelectionReceived(GPIO_PIN signal)
{
    if (GPIO_PIN_NAMES.find(signal)==GPIO_PIN_NAMES.end()) return;
    unsigned int i=0;
    while (i<ui->adcTriggerSelectionComboBox->count()) {
        if (ui->adcTriggerSelectionComboBox->itemText(i).compare(GPIO_PIN_NAMES[signal])==0) break;
        i++;
    }
    if (i>=ui->adcTriggerSelectionComboBox->count()) return;
    ui->adcTriggerSelectionComboBox->blockSignals(true);
    ui->adcTriggerSelectionComboBox->setEnabled(true);
    ui->adcTriggerSelectionComboBox->setCurrentIndex(i);
    ui->adcTriggerSelectionComboBox->blockSignals(false);
}

void ParameterMonitorForm::onGainSwitchReceived(bool state)
{
    ui->hiGainCheckBox->setChecked(state);
}

void ParameterMonitorForm::onTemperatureReceived(float temp)
{
    ui->temperatureLabel->setText(QString::number(temp, 'f', 2));
}

void ParameterMonitorForm::onTimepulseReceived()
{
    //
}

void ParameterMonitorForm::onAdcTraceReceived(const QVector<float> &sampleBuffer)
{
    //
    QVector<QPointF> vec;
    for (int i=0; i<sampleBuffer.size(); i++) {
        QPointF p1;
        p1.rx()=i-10;
        p1.ry()=sampleBuffer[i];
        vec.push_back(p1);
    }

    ui->adcTracePlot->curve("curve1").setSamples(vec);
    ui->adcTracePlot->replot();

}

void ParameterMonitorForm::onTimeAccReceived(quint32 acc)
{
    ui->timePrecLabel->setText(QString::number(acc*1e-9, 'g', 6));
}

void ParameterMonitorForm::onFreqAccReceived(quint32 acc)
{//

}

void ParameterMonitorForm::onIntCounterReceived(quint32 cnt)
{
    ui->ubloxCounterLabel->setText(QString::number(cnt));
}


void ParameterMonitorForm::on_dacSpinBox1_valueChanged(double arg1)
{
    emit setDacVoltage(0, arg1);
}

void ParameterMonitorForm::on_dacSpinBox2_valueChanged(double arg1)
{
    emit setDacVoltage(1, arg1);
}

void ParameterMonitorForm::on_dacSpinBox3_valueChanged(double arg1)
{
    emit setDacVoltage(2, arg1);
}

void ParameterMonitorForm::on_dacSpinBox4_valueChanged(double arg1)
{
    emit setDacVoltage(3, arg1);
}

void ParameterMonitorForm::on_dacSlider1_valueChanged(int value)
{
    //
    double voltage = value/1000.;
    emit setDacVoltage(0, voltage);
}

void ParameterMonitorForm::on_dacSlider2_valueChanged(int value)
{
    //
    double voltage = value/1000.;
    emit setDacVoltage(1, voltage);
}

void ParameterMonitorForm::on_dacSlider3_valueChanged(int value)
{
    //
    double voltage = value/1000.;
    emit setDacVoltage(2, voltage);
}

void ParameterMonitorForm::on_dacSlider4_valueChanged(int value)
{
    //
    double voltage = value/1000.;
    emit setDacVoltage(3, voltage);
}
