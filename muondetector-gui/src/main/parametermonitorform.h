#ifndef PARAMETERMONITORFORM_H
#define PARAMETERMONITORFORM_H

#include <QWidget>
#include <gpio_pin_definitions.h>

struct CalibStruct;

namespace Ui {
class ParameterMonitorForm;
}

class ParameterMonitorForm : public QWidget
{
    Q_OBJECT

public:
    explicit ParameterMonitorForm(QWidget *parent = 0);
    ~ParameterMonitorForm();

signals:
    void setDacVoltage(uint8_t ch, float val);

public slots:
    void onAdcSampleReceived(uint8_t channel, float value);
    void onDacReadbackReceived(uint8_t channel, float value);
    void onInputSwitchReceived(uint8_t index);
    void onBiasSwitchReceived(bool state);
    void onPreampSwitchReceived(uint8_t channel, bool state);
    void onTriggerSelectionReceived(GPIO_PIN signal);
    void onGainSwitchReceived(bool state);
    void onTemperatureReceived(float temp);
    void onTimepulseReceived();
    void onAdcTraceReceived(const QVector<float>& sampleBuffer);
    void onTimeAccReceived(quint32 acc);
    void onFreqAccReceived(quint32 acc);
    void onIntCounterReceived(quint32 cnt);
    void onCalibReceived(bool valid, bool eepromValid, quint64 id, const QVector<CalibStruct> &calibList);

private slots:
    void on_dacSpinBox1_valueChanged(double arg1);

    void on_dacSpinBox2_valueChanged(double arg1);

    void on_dacSpinBox3_valueChanged(double arg1);

    void on_dacSpinBox4_valueChanged(double arg1);

private:
    Ui::ParameterMonitorForm *ui;
    QVector<CalibStruct> fCalibList;
};

#endif // PARAMETERMONITORFORM_H
