#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QErrorMessage>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QTime>
#include <QTimer>
#include <QVector>

// for sig handling:
#include <config.h>
#include <events/tcp_packet_event.h>
#include <events/ubx_event.h>
#include <functional>
#include <gpio_pin_definitions.h>
#include <mqtthandler.h>
#include <sys/types.h>
#include <unordered_map>

struct I2cDeviceEntry;
struct CalibStruct;
struct GnssPosStruct;
struct PositionModeConfig;
struct GnssConfigStruct;
class GnssSatellite;
class CalibForm;
class CalibScanDialog;
struct UbxTimePulseStruct;
class Histogram;
struct GnssMonHwStruct;
struct GnssMonHw2Struct;
struct LogInfoStruct;
struct UbxTimeMarkStruct;
class TcpConnection;
enum class ADC_SAMPLING_MODE;
enum class TCP_MSG_KEY : quint16;

namespace Ui {
class MainWindow;
}

namespace boost {
namespace asio {
class io_context;
}
} // namespace boost

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(std::shared_ptr<boost::asio::io_context> io, QWidget* parent = 0);
    ~MainWindow();

  signals:
    void addUbxMsgRates(const UbxMsgRates& rates);
    void gpioRates(quint8 whichrate, QVector<QPointF> rate);
    void tcpDisconnected();
    void setUiEnabledStates(bool enabled);
    void geodeticPos(const GnssPosStruct& pos);
    void positionModeConfigReceived(const PositionModeConfig& posconfig);
    void adcSampleReceived(uint8_t channel, float value);
    void adcTraceReceived(const std::vector<float>& sampleBuffer);
    void inputSwitchReceived(TIMING_MUX_SELECTION);
    void dacReadbackReceived(uint8_t channel, float value);
    void biasSwitchReceived(bool state);
    void preampSwitchReceived(uint8_t channel, bool state);
    void gainSwitchReceived(bool state);
    void temperatureReceived(float temp);
    void i2cStatsReceived(quint32 bytesRead, quint32 bytesWritten,
                          const std::vector<I2cDeviceEntry>& deviceList);
    void spiStatsReceived(bool spiPresent);
    void calibReceived(bool valid, bool eepromValid, quint64 id,
                       const std::vector<CalibStruct>& calibList);
    void satsReceived(const std::vector<GnssSatellite>& satList);
    void gnssConfigsReceived(quint8 numTrkCh, const std::vector<GnssConfigStruct>& configList);
    void timeAccReceived(quint32 acc);
    void freqAccReceived(quint32 acc);
    void intCounterReceived(quint32 cnt);
    void txBufReceived(quint8 val);
    void txBufPeakReceived(quint8 val);
    void rxBufReceived(quint8 val);
    void rxBufPeakReceived(quint8 val);
    void gpsMonHWReceived(const GnssMonHwStruct& hwstruct);
    void gpsMonHW2Received(const GnssMonHw2Struct& hw2struct);
    void gpsVersionReceived(const QString& swString, const QString& hwString,
                            const QString& protString);
    void gpsFixReceived(quint8 val);
    void ubxUptimeReceived(quint32 val);
    void gpsTP5Received(const UbxTimePulseStruct& tp);
    void histogramReceived(const Histogram& h);
    void triggerSelectionReceived(GPIO_SIGNAL signal);
    void timepulseReceived();
    void adcModeReceived(quint8 mode);
    void logInfoReceived(const LogInfoStruct& lis);
    void mqttStatusChanged(bool connected);
    void mqttStatusChanged(MuonPi::MqttHandler::Status status);
    void timeMarkReceived(const UbxTimeMarkStruct&);
    void polaritySwitchReceived(bool pol1, bool pol2);
    void gpioInhibitReceived(bool inhibit);
    void mqttInhibitReceived(bool inhibit);
    void daemonVersionReceived(MuonPi::Version::Version hw_ver, MuonPi::Version::Version sw_ver);

  public slots:
    void sendRequestUbxMsgRates();
    void sendSetUbxMsgRateChanges(QMap<uint16_t, int> changes);
    void onSendUbxReset();
    void makeConnection(QString ipAddress, quint16 port);
    void onTriggerSelectionChanged(GPIO_SIGNAL signal);
    void onHistogramCleared(QString histogramName);
    void onAdcModeChanged(ADC_SAMPLING_MODE mode);
    void onRateScanStart(uint8_t ch);
    void gpioInhibit(bool inhibit);
    void mqttInhibit(bool inhibit);
    void onPolarityChanged(bool pol1, bool pol2);
    void onPosModeConfigChanged(const PositionModeConfig& posconfig);

  private slots:
    void resetAndHit();
    void resetXorHit();
    void sendRequestGpioRates();
    void sendRequestGpioRateBuffer();
    void sendValueUpdateRequests();
    void sendPreamp1Switch(bool status);
    void sendPreamp2Switch(bool status);
    void sendSetBiasStatus(bool status);

    void onIpButtonClicked();
    void connected();
    void connection_info(const QString message);
    void connection_error(int error_code, const QString message);
    void sendInputSwitch(TIMING_MUX_SELECTION sel);

    void on_discr1Save_clicked();
    void on_discr2Save_clicked();

    void on_biasPowerButton_clicked();
    void on_biasVoltageSlider_sliderReleased();
    void on_biasVoltageSlider_valueChanged(int value);
    void on_biasVoltageSlider_sliderPressed();
    void onCalibUpdated(const std::vector<CalibStruct>& items);
    void on_biasControlTypeComboBox_currentIndexChanged(int index);
    void onSetGnssConfigs(const std::vector<GnssConfigStruct>& configList);
    void onSetTP5Config(const UbxTimePulseStruct& tp);
    void on_biasVoltageDoubleSpinBox_valueChanged(double arg1);
    void on_saveDacButton_clicked();
    void onBiasSwitchReceived(bool biasEnabled);

  private:
    void decode(const TcpPacket& packet);
    std::unordered_map<TCP_MSG_KEY, std::function<void(const TcpPacket&)>> decoderMap;
    auto
    buildDecoderMap() -> std::unordered_map<TCP_MSG_KEY, std::function<void(const TcpPacket&)>>;
    Ui::MainWindow* ui;
    void uiSetConnectedState();
    void uiSetDisconnectedState();
    float parseValue(QString text);
    void sendSetBiasVoltage(float voltage);
    void sendSetThresh(uint8_t channel, float value);
    void setMaxThreshVoltage(float voltage);
    void sendGainSwitch(bool status);

    void updateUiProperties();
    int verbose = 0;
    float biasDacVoltage = 0.;
    bool biasON, uiValuesUpToDate = false;
    quint8 pcaPortMask = 0;
    std::array<double, 2> sliderValues{0, 0};
    bool sliderValuesDirty{true};
    float maxThreshVoltage;
    QErrorMessage errorM;
    QStandardItemModel* addresses;
    QList<QStandardItem*>* addressColumn;
    bool saveSettings(QStandardItemModel* model);
    bool loadSettings(QStandardItemModel* model);
    bool eventFilter(QObject* object, QEvent* event);
    bool mouseHold = false;
    bool automaticRatePoll = true;
    QTimer andTimer, xorTimer, ratePollTimer;
    CalibForm* calib = nullptr;
    CalibScanDialog* calibscandialog = nullptr;
    double biasCalibOffset = 0.;
    double biasCalibSlope = 1.;
    double minBiasVoltage = 0.;
    double maxBiasVoltage = 3.3;
    QTimer m_connection_timeout{};
    std::shared_ptr<boost::asio::io_context> m_io{nullptr};
    std::shared_ptr<TcpConnection> clientConn{nullptr};
};

#endif // MAINWINDOW_H
