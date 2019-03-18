#ifndef STATUS_H
#define STATUS_H

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QPointF>
#include <QButtonGroup>

namespace Ui {
class Status;
}

class Status : public QWidget
{
    Q_OBJECT

public:
    explicit Status(QWidget *parent = nullptr);
    ~Status();

signals:
	void inputSwitchChanged(int id);
	void biasSwitchChanged(bool state);
	void gainSwitchChanged(bool state);
	void preamp1SwitchChanged(bool state);
	void preamp2SwitchChanged(bool state);
    void resetRateClicked();
public slots:
    void onGpioRatesReceived(quint8 whichrate, QVector<QPointF> rates);
    void onAdcSampleReceived(uint8_t channel, float value);
    void onUiEnabledStateChange(bool connected);
    void updatePulseHeightHistogram();
   	void on_histoLogYCheckBox_clicked();
   	void onInputSwitchReceived(uint8_t id);
   	void onBiasSwitchReceived(bool state);
   	void onGainSwitchReceived(bool state);
   	void onPreampSwitchReceived(uint8_t channel, bool state);
   	void onDacReadbackReceived(uint8_t channel, float value);
    void onTemperatureReceived(float value);
   	void clearPulseHeightHisto();
    void clearRatePlot();

private slots:
    void setRateSecondsBuffered(const QString& bufferTime);

private:
    Ui::Status *statusUi;
    QVector<QPointF> xorSamples;
    QVector<QPointF> andSamples;
    QButtonGroup* fInputSwitchButtonGroup;
};

#endif // STATUS_H
