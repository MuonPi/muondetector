#ifndef STATUS_H
#define STATUS_H

#include <QButtonGroup>
#include <QMap>
#include <QPointF>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <gpio_pin_definitions.h>
#include <mqtthandler.h>

namespace Ui {
class Status;
}

class Status : public QWidget {
    Q_OBJECT

public:
    explicit Status(QWidget* parent = nullptr);
    ~Status();

signals:
    void inputSwitchChanged(TIMING_MUX_SELECTION sel);
    void biasSwitchChanged(bool state);
    void gainSwitchChanged(bool state);
    void preamp1SwitchChanged(bool state);
    void preamp2SwitchChanged(bool state);
    void resetRateClicked();
    void triggerSelectionChanged(GPIO_SIGNAL signal);

public slots:
    void onGpioRatesReceived(quint8 whichrate, QVector<QPointF> rates);
    void onAdcSampleReceived(uint8_t channel, float value);
    void onUiEnabledStateChange(bool connected);
    void updatePulseHeightHistogram();
    void on_histoLogYCheckBox_clicked();
    void onInputSwitchReceived(TIMING_MUX_SELECTION sel);
    void onBiasSwitchReceived(bool state);
    void onGainSwitchReceived(bool state);
    void onPreampSwitchReceived(uint8_t channel, bool state);
    void onDacReadbackReceived(uint8_t channel, float value);
    void onTemperatureReceived(float value);
    void clearPulseHeightHisto();
    void clearRatePlot();
    void onTriggerSelectionReceived(GPIO_SIGNAL signal);
    void onTimepulseReceived();
    void onMqttStatusChanged(bool connected);
    void onMqttStatusChanged(MuonPi::MqttHandler::Status status);

private slots:
    void setRateSecondsBuffered(const QString& bufferTime);
    void on_timingSelectionComboBox_currentIndexChanged(const QString& arg);
    void on_triggerSelectionComboBox_currentIndexChanged(const QString& arg1);

private:
    Ui::Status* statusUi;
    QVector<QPointF> xorSamples;
    QVector<QPointF> andSamples;
    QTimer timepulseTimer;
    static constexpr quint64 rateSecondsBufferedDefault { 60 * 120 }; // 120 min
    quint64 rateSecondsBuffered { rateSecondsBufferedDefault };
};

#endif // STATUS_H
