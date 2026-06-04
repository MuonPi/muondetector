#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "data/events/mqtt_status_event.h"
#include "data/events/ubx_event.h"
#include "data/muondetector_structs.h"

#include <QErrorMessage>
#include <QMainWindow>
#include <QStandardItemModel>
#include <QTime>
#include <QTimer>
#include <QVector>
#include <vector>
// for sig handling:
#include <config.h>
#include <events/tcp_packet_event.h>
#include <events/ubx_event.h>
#include <functional>
#include <gpio_pin_definitions.h>
#include <sys/types.h>
#include <unordered_map>

class CalibForm;
class CalibScanDialog;
class TcpConnection;
class NetworkDiscovery;
enum class ADC_SAMPLING_MODE;
enum class TCP_MSG_KEY : quint16;

namespace Ui {
class MainWindow;
}

namespace boost {
namespace asio {
class io_context;
namespace tcp {
class socket;
}
} // namespace asio
} // namespace boost

class MainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(std::shared_ptr<boost::asio::io_context> io, QWidget* parent = 0);
    ~MainWindow();

  signals:
    void addCfgMsgRate(const CfgMsg& rates);
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
    void mqttStatusChanged(const MqttStatusEvent& status);
    void timeMarkReceived(const UbxTimeMarkStruct&);
    void polaritySwitchReceived(bool pol1, bool pol2);
    void gpioInhibitReceived(bool inhibit);
    void mqttInhibitReceived(bool inhibit);
    void daemonVersionReceived(MuonPi::Version::Version hw_ver, MuonPi::Version::Version sw_ver);
    void deviceDiscovered(QString ip);
    void gpioEventReceived(GPIO_SIGNAL, EventEdge);

  public slots:
    void sendRequestCfgMsgRates();
    void sendSetCfgMsgRateChange(uint16_t key, int rate);
    void onSendUbxReset();
    void makeConnection(QString ipAddress, quint16 port);
    void onTriggerSelectionChanged(GPIO_SIGNAL signal);
    void onHistogramCleared(QString histogramName);
    void sendSetAdcMode(ADC_SAMPLING_MODE mode);
    void sendRateScanStart(uint8_t ch);
    void gpioInhibit(bool inhibit);
    void mqttInhibit(bool inhibit);
    void onPolarityChanged(bool pol1, bool pol2);
    void onPosModeConfigChanged(const PositionModeConfig& posconfig);

  private slots:
    void resetAndHit();
    void resetXorHit();
    void cancelConnection();
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

    void discr1SaveClicked();
    void discr2SaveClicked();

    void onBiasPowerButtonClicked();
    void onBiasVoltageSliderSliderReleased();
    void onBiasVoltageSliderValueChanged(int value);
    void onBiasVoltageSliderSliderPressed();
    void onCalibUpdated(const std::vector<CalibStruct>& items);
    void onBiasControlTypeComboBoxCurrentIndexChanged(int index);
    void onSetGnssConfigs(const std::vector<GnssConfigStruct>& configList);
    void onSetTP5Config(const UbxTimePulseStruct& tp);
    void onBiasVoltageDoubleSpinBoxValueChanged(double arg1);
    void onSaveDacButtonClicked();
    void onBiasSwitchReceived(bool biasEnabled);

  private:
    Ui::MainWindow* ui;
    std::unordered_map<TCP_MSG_KEY, std::function<void(const TcpPacket&)>> decoderMap;
    auto
    buildDecoderMap() -> std::unordered_map<TCP_MSG_KEY, std::function<void(const TcpPacket&)>>;
    void decode(const TcpPacket& packet);
    void uiSetConnectedState();
    void uiSetConnectingState();
    void uiSetDisconnectedState();
    float parseValue(QString text);
    void sendSetBiasVoltage(float voltage);
    void sendSetThresh(uint8_t channel, float value);
    void setMaxThreshVoltage(float voltage);
    void sendGainSwitch(bool status);

    void updateUiProperties();
    int connectProgress_ = 100;
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
    bool automaticRatePoll = false;
    QTimer andTimer, xorTimer, ratePollTimer;
    CalibForm* calib = nullptr;
    CalibScanDialog* calibscandialog = nullptr;
    double biasCalibOffset = 0.;
    double biasCalibSlope = 1.;
    double minBiasVoltage = 0.;
    double maxBiasVoltage = 3.3;
    // QTimer m_connection_timeout{};
    std::shared_ptr<tcp::socket> clientSocket_{nullptr};
    std::shared_ptr<boost::asio::io_context> m_io{nullptr};
    std::shared_ptr<TcpConnection> clientConn{nullptr};
    std::size_t device_counter{0};
    std::shared_ptr<NetworkDiscovery> m_networkDiscovery{nullptr};
    QTimer* connectTimer_ = nullptr;
    bool connecting_{false};
};

#endif // MAINWINDOW_H
