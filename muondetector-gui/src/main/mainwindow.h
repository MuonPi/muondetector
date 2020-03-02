#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <tcpconnection.h>
#include <QStandardItemModel>
#include <QErrorMessage>
#include <QTime>
#include <QVector>
//#include <geodeticpos.h>
#include <gpio_pin_definitions.h>

// for sig handling:
#include <sys/types.h>

struct I2cDeviceEntry;
struct CalibStruct;
struct GeodeticPos;
struct GnssConfigStruct;
class GnssSatellite;
class CalibForm;
class CalibScanDialog;
struct UbxTimePulseStruct;
class Histogram;

namespace Ui {
	class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

signals:
	void addUbxMsgRates(QMap<uint16_t, int> ubxMsgRates);
	void sendTcpMessage(TcpMessage tcpMessage);
    void closeConnection();
    void gpioRates(quint8 whichrate, QVector<QPointF> rate);
    void tcpDisconnected();
    void setUiEnabledStates(bool enabled);
    void geodeticPos(const GeodeticPos& pos);
    void adcSampleReceived(uint8_t channel, float value);
    void adcTraceReceived(const QVector<float>& sampleBuffer);
    void inputSwitchReceived(uint8_t);
    void dacReadbackReceived(uint8_t channel, float value);
    void biasSwitchReceived(bool state);
    void preampSwitchReceived(uint8_t channel, bool state);
    void gainSwitchReceived(bool state);
    void temperatureReceived(float temp);
	void i2cStatsReceived(quint32 bytesRead, quint32 bytesWritten, const QVector<I2cDeviceEntry>& deviceList);
    void spiStatsReceived(bool spiPresent);
	void calibReceived(bool valid, bool eepromValid, quint64 id, const QVector<CalibStruct>& calibList);
	void satsReceived(const QVector<GnssSatellite>& satList);
    void gnssConfigsReceived(quint8 numTrkCh, const QVector<GnssConfigStruct>& configList);
	void timeAccReceived(quint32 acc);
    void freqAccReceived(quint32 acc);
    void intCounterReceived(quint32 cnt);
	void txBufReceived(quint8 val);
	void txBufPeakReceived(quint8 val);
    void rxBufReceived(quint8 val);
    void rxBufPeakReceived(quint8 val);
    void gpsMonHWReceived(quint16 noise, quint16 agc, quint8 antStatus, quint8 antPower, quint8 jamInd, quint8 flags);
    void gpsMonHW2Received(qint8 ofsI, quint8 magI, qint8 ofsQ, quint8 magQ, quint8 cfgSrc);
    void gpsVersionReceived(const QString& swString, const QString& hwString, const QString& protString);
	void gpsFixReceived(quint8 val);
    void ubxUptimeReceived(quint32 val);
    void gpsTP5Received(const UbxTimePulseStruct& tp);
    void histogramReceived(const Histogram& h);
    void triggerSelectionReceived(GPIO_PIN signal);
    void timepulseReceived();

public slots:
	void receivedTcpMessage(TcpMessage tcpMessage);
    void receivedGpioRisingEdge(GPIO_PIN pin);
    void sendRequestUbxMsgRates();
    void sendSetUbxMsgRateChanges(QMap<uint16_t, int> changes);
    void onSendUbxReset();
	void makeConnection(QString ipAddress, quint16 port);
    void onTriggerSelectionChanged(GPIO_PIN signal);
    void onHistogramCleared(QString histogramName);

private slots:
	// only those properties with value >= 0 will be updated!
	void resetAndHit();
	void resetXorHit();
    void sendRequestGpioRates();
    void sendRequestGpioRateBuffer();
	void sendValueUpdateRequests();

	void on_ipButton_clicked();
	void connected();
	void sendInputSwitch(int id);

	void on_discr1Edit_editingFinished();

	void on_discr1Slider_sliderReleased();

	void on_discr1Slider_valueChanged(int value);

	void on_discr1Slider_sliderPressed();

	void on_discr2Slider_sliderReleased();

	void on_discr2Slider_valueChanged(int value);

	void on_discr2Slider_sliderPressed();

	void on_biasPowerButton_clicked();

	void on_discr2Edit_editingFinished();

    void on_biasVoltageSlider_sliderReleased();

    void on_biasVoltageSlider_valueChanged(int value);

    void on_biasVoltageSlider_sliderPressed();
    void onCalibUpdated(const QVector<CalibStruct>& items);

    void on_biasControlTypeComboBox_currentIndexChanged(int index);
    void onSetGnssConfigs(const QVector<GnssConfigStruct>& configList);
    void onSetTP5Config(const UbxTimePulseStruct& tp);

    void on_biasVoltageDoubleSpinBox_valueChanged(double arg1);

    void on_saveDacButton_clicked();

private:
	Ui::MainWindow *ui;
	void uiSetConnectedState();
	void uiSetDisconnectedState();
	float parseValue(QString text);
    void sendRequest(quint16 requestSig);
    void sendRequest(quint16 requestSig, quint8 par);
    void sendSetBiasVoltage(float voltage);
    void sendSetBiasStatus(bool status);
    void sendSetThresh(uint8_t channel, float value);
    void setMaxThreshVoltage(float voltage);
    void sendPreamp1Switch(bool status);
    void sendPreamp2Switch(bool status);
    void sendGainSwitch(bool status);

    void updateUiProperties();
    int verbose = 0;
    float biasDacVoltage = 0.;
    bool biasON, uiValuesUpToDate = false;
    quint8 pcaPortMask = 0;
    QVector<int> sliderValues = QVector<int>({0,0});
    float maxThreshVoltage;
	QErrorMessage errorM;
	TcpConnection *tcpConnection = nullptr;
	QStandardItemModel *addresses;
	QList<QStandardItem *> *addressColumn;
    bool saveSettings(QStandardItemModel* model);
    bool loadSettings(QStandardItemModel* model);
	bool eventFilter(QObject *object, QEvent *event);
	bool connectedToDemon = false;
	bool mouseHold = false;
    bool automaticRatePoll = true;
    QTimer andTimer, xorTimer, ratePollTimer;
    CalibForm *calib = nullptr;
    CalibScanDialog *calibscandialog = nullptr;
    double biasCalibOffset = 0.;
    double biasCalibSlope = 1.;
    double minBiasVoltage = 0.;
    double maxBiasVoltage = 3.3;
};

#endif // MAINWINDOW_H
