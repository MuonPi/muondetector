#ifndef PARAMETERMONITORFORM_H
#define PARAMETERMONITORFORM_H

#include <QWidget>
#include <gpio_pin_definitions.h>
#include <muondetector_structs.h>
#include <vector>

struct CalibStruct;
struct UbxTimeMarkStruct;

namespace Ui {
class ParameterMonitorForm;
}

class ParameterMonitorForm : public QWidget {
    Q_OBJECT

  public:
    explicit ParameterMonitorForm(QWidget* parent = 0);
    ~ParameterMonitorForm();

  signals:
    void setDacVoltage(uint8_t ch, float val);
    void biasVoltageCalculated(float vbias);
    void biasCurrentCalculated(float ibias);
    void setAdcMode(ADC_SAMPLING_MODE mode);
    void gpioInhibitChanged(bool inhibitState);
    void mqttInhibitChanged(bool inhibitState);
    void biasEnableChanged(bool state);
    void preamp1EnableChanged(bool state);
    void preamp2EnableChanged(bool state);
    void polarityChanged(bool pol1, bool pol2);
    void timingSelectionChanged(TIMING_MUX_SELECTION sel);
    void triggerSelectionChanged(GPIO_SIGNAL signal);
    void gainSwitchChanged(bool state);

  public slots:
    void onCalibReceived(bool valid, bool eepromValid, quint64 id,
                         const std::vector<CalibStruct>& calibList);
    void onAdcSampleReceived(uint8_t channel, float value);
    void onDacReadbackReceived(uint8_t channel, float value);
    void onInputSwitchReceived(TIMING_MUX_SELECTION sel);
    void onBiasSwitchReceived(bool state);
    void onPreampSwitchReceived(uint8_t channel, bool state);
    void onTriggerSelectionReceived(GPIO_SIGNAL signal);
    void onGainSwitchReceived(bool state);
    void onTemperatureReceived(float temp);
    void onTimepulseReceived();
    void onAdcTraceReceived(const std::vector<float>& sampleBuffer);
    void onTimeAccReceived(quint32 acc);
    void onFreqAccReceived(quint32 acc);
    void onIntCounterReceived(quint32 cnt);
    void onTimeMarkReceived(const UbxTimeMarkStruct& tm);
    void onPolaritySwitchReceived(bool pol1, bool pol2);
    void onUiEnabledStateChange(bool connected);
    void onDaemonVersionReceived(MuonPi::Version::Version hw_ver, MuonPi::Version::Version sw_ver);

  private slots:
    void onDacSpinBox1ValueChanged(double arg1);
    void onDacSpinBox2ValueChanged(double arg1);
    void onDacSpinBox3ValueChanged(double arg1);
    void onDacSpinBox4ValueChanged(double arg1);
    void onDacSlider1ValueChanged(int value);
    void onDacSlider2ValueChanged(int value);
    void onDacSlider3ValueChanged(int value);
    void onDacSlider4ValueChanged(int value);
    void onGpioInhibitCheckBoxClicked(bool checked);
    void onMqttInhibitCheckBoxClicked(bool checked);
    void onPolarityCheckBoxClicked(bool checked);
    void onTimingSelectionComboBoxCurrentIndexChanged(int index);
    void onAdcTriggerSelectionComboBoxCurrentIndexChanged(int index);

  private:
    Ui::ParameterMonitorForm* ui;
    std::vector<CalibStruct> fCalibList;

    double fLastBiasVoltageLo = -999.;
    double fLastBiasVoltageHi = -999.;

    QString getCalibParameter(const QString& name);
    bool currentCalibValid();
};

#endif // PARAMETERMONITORFORM_H
