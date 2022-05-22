#ifndef DAEMON_H
#define DAEMON_H

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QSocketNotifier>
#include <QTcpServer>
#include <QTimer>
#include <QVariant>
#include <time.h>
#include <memory>

// clang-format off
#include "qtserialublox.h"
#include "utility/filehandler.h"
#include "utility/kalman_gnss_filter.h"
#include "calibration.h"
// clang-format on

#include <custom_io_operators.h>
#include "logengine.h"
#include "logparameter.h"
#include "pigpiodhandler.h"
#include "hardware/spidevices.h"
#include "hardware/device_types.h"

// from library
#include <muondetector_structs.h>
#include <histogram.h>
#include <mqtthandler.h>
#include <tcpconnection.h>

// for sig handling:
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

enum GPIO_SIGNAL;

class Daemon : public QTcpServer {
    Q_OBJECT

public:
    struct configuration {
        QString username;
        QString password;
        QString gpsdevname { "" };
        int verbose { 0 };
        quint8 pcaPortMask { 0 };
        std::array<float, 2> thresholdVoltage { -1.0F, -1.0F };
        float biasVoltage { -1.0F };
        bool bias_ON { false };
        GPIO_SIGNAL eventTrigger { EVT_XOR };
        QString peerAddress { "" };
        quint16 peerPort { 0 };
        QString serverAddress { "" };
        quint16 serverPort { 0 };
        bool showout { false };
        bool showin { false };
        std::array<bool, 2> preamp_enable { false, false };
        bool hi_gain { false };
        QString station_ID { "0" };
        std::array<bool, 2> polarity { true, true };
        int maxGeohashLength { MuonPi::Settings::log.max_geohash_length };
        bool storeLocal { false };
        /* GNSS configs */
        bool gnss_dump_raw { false };
        int gnss_baudrate { 9600 };
        bool gnss_config { false };
        UbxDynamicModel gnss_dynamic_model { UbxDynamicModel::stationary };
    };

    Daemon(configuration cfg, QObject* parent = nullptr);

    ~Daemon() override;
    void configGps();
    void configGpsForVersion();
    void loop();
    static void hupSignalHandler(int);
    static void termSignalHandler(int);
    static void intSignalHandler(int);

public slots:
    // Qt signal handlers.
    void handleSigHup();
    void handleSigTerm();
    void handleSigInt();
    // others
    void connectToPigpiod();
    void connectToGps();
    void displaySocketError(int socketError, QString message);
    void displayError(QString message);
    void toConsole(const QString& data);
    void gpsToConsole(const QString& data);
    void onMadeConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort);
    void onStoppedConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort, quint32 timeoutTime, quint32 connectionDuration);
    void UBXReceivedAckNak(uint16_t ackedMsgID, uint16_t ackedCfgMsgID);
    void UBXReceivedMsgRateCfg(uint16_t msgID, uint8_t rate);
    void gpsConnectionError();
    void gpsPropertyUpdatedInt32(int32_t data, std::chrono::duration<double> updateAge, char propertyName);
    void gpsPropertyUpdatedUint32(uint32_t data, std::chrono::duration<double> updateAge, char propertyName);
    void gpsPropertyUpdatedUint8(uint8_t data, std::chrono::duration<double> updateAge, char propertyName);
    void onUBXReceivedTxBuf(uint8_t txUsage, uint8_t txPeakUsage);
    void onUBXReceivedRxBuf(uint8_t rxUsage, uint8_t rxPeakUsage);
    void onGpsPropertyUpdatedGnss(const std::vector<GnssSatellite>& sats, std::chrono::duration<double> lastUpdated);
    void onUBXReceivedGnssConfig(uint8_t numTrkCh, const std::vector<GnssConfigStruct>& gnssConfigs);
    void onUBXReceivedTP5(const UbxTimePulseStruct& tp);
    void onGpsMonHWUpdated(const GnssMonHwStruct& hw);
    void onGpsMonHW2Updated(const GnssMonHw2Struct& hw2);
    void receivedTcpMessage(TcpMessage tcpMessage);
    void pollAllUbxMsgRate();
    void sendGpioPinEvent(uint8_t gpio_pin);
    void onGpsPropertyUpdatedGeodeticPos(const GeodeticPos& pos);
    void UBXReceivedVersion(const QString& swString, const QString& hwString, const QString& protString);
    void sampleAdc0Event();
    void sampleAdc0TraceEvent();
    void sampleAdcEvent(uint8_t channel);
    void getTemperature();
    void scanI2cBus();
    void onUBXReceivedTimeTM2(const UbxTimeMarkStruct& tm);
    void onLogParameterPolled();
    void sendExtendedMqttStatus(MuonPi::MqttHandler::Status status);

signals:
    void sendTcpMessage(TcpMessage tcpMessage);
    void closeConnection(QString closeAddress);
    void logParameter(const LogParameter& log);
    void aboutToQuit();
    void sendPollUbxMsgRate(uint16_t msgID);
    void sendPollUbxMsg(uint16_t msgID);
    void sendUbxMsg(uint16_t msgID, const std::string& data);
    // difference between msgRate and msg is that CFG-MSG (0x0601) alone is used
    // to set/get the rate for every message so the msgID must be wrapped to the data
    // of a message of type CFG-MSG first
    void UBXSetCfgMsgRate(uint16_t msgID, uint8_t port, uint8_t rate);
    void UBXSetCfgRate(uint8_t measRate, uint8_t navRate);
    void UBXSetCfgPrt(uint8_t gpsPort, uint8_t outProtocolMask);
    void UBXSetDynModel(UbxDynamicModel model);
    void resetUbxDevice(uint32_t flags);
    void setGnssConfig(const std::vector<GnssConfigStruct>& gnssConfigs);
    void UBXSetMinMaxSVs(uint8_t minSVs, uint8_t maxSVs);
    void UBXSetMinCNO(uint8_t minCNO);
    void GpioSetInput(unsigned int gpio);
    void GpioSetOutput(unsigned int gpio);
    void GpioSetPullUp(unsigned int gpio);
    void GpioSetPullDown(unsigned int gpio);
    void GpioSetState(unsigned int gpio, bool state);
    void GpioRegisterForCallback(unsigned int gpio, bool edge); // false=falling, true=rising
    void UBXSetCfgTP5(const UbxTimePulseStruct& tp);
    void UBXSetAopCfg(bool enable = true, uint16_t maxOrbErr = 0);
    void UBXSaveCfg(uint8_t devMask = QtSerialUblox::DEV_BBR | QtSerialUblox::DEV_FLASH);
    void setSamplingTriggerSignal(GPIO_SIGNAL signalName);
    void timeMarkIntervalCountUpdate(uint16_t newCounts, double lastInterval);
    void requestMqttConnectionStatus();
    void eventMessage(const QString& messageString);

private slots:
    void onRateBufferReminder();
    void updateOledDisplay();
    void aquireMonitoringParameters();
    void onStatusLed1Event(int onTimeMs);
    void onStatusLed2Event(int onTimeMs);

private:
    void incomingConnection(qintptr socketDescriptor) override;
    void setPcaChannel(uint8_t channel); // channel 0 to 3
        // 0: coincidence ; 1: xor ; 2: discr 1 ; 3: discr 2
    void setEventTriggerSelection(GPIO_SIGNAL signal);
    void sendPcaChannel();
    void sendVersionInfo();
    void sendEventTriggerSelection();
    void setDacThresh(uint8_t channel, float threshold); // channel 0 or 1 ; threshold in volts
    void sendDacThresh(uint8_t channel);
    void sendDacReadbackValue(uint8_t channel, float voltage);
    void setBiasVoltage(float voltage);
    void saveDacValuesToEeprom();
    void sendBiasVoltage();
    void sendBiasStatus();
    void sendGainSwitchStatus();
    void setBiasStatus(bool status);
    void sendPreampStatus(uint8_t channel);
    void sendPolarityStatus();
    void setUbxMsgRates(QMap<uint16_t, int>& ubxMsgRates);
    void sendUbxMsgRates();
    void sendGpioRates(int number = 0, quint8 whichRate = 0);
    void sendI2cStats();
    void sendSpiStats();
    void sendCalib();
    void sendHistogram(const Histogram& hist);
    void sendLogInfo();
    bool readEeprom();
    void receivedCalibItems(const std::vector<CalibStruct>& newCalibs);
    void setupHistos();
    void rescaleHisto(Histogram& hist, double center, double width);
    void rescaleHisto(Histogram& hist, double center);
    void clearHisto(const QString& histoName);
    void setAdcSamplingMode(ADC_SAMPLING_MODE mode);
    void printTimestamp();
    void delay(int millisecondsWait);
    void onAdcSampleReady(ADS1115::Sample sample);

    void rateCounterIntervalActualisation();
    qreal getRateFromCounts(quint8 which_rate);
    void clearRates();

    std::shared_ptr<DeviceFunction<DeviceType::TEMP>> temp_sensor_p;
    std::shared_ptr<DeviceFunction<DeviceType::ADC>> adc_p;
    std::shared_ptr<DeviceFunction<DeviceType::DAC>> dac_p;
    std::shared_ptr<DeviceFunction<DeviceType::EEPROM>> eep_p;
    std::shared_ptr<DeviceFunction<DeviceType::IO_EXTENDER>> io_extender_p;
    std::shared_ptr<Adafruit_SSD1306> oled_p {};
    std::shared_ptr<UbloxI2c> ublox_i2c_p {};

    QPointer<PigpiodHandler> pigHandler;
    QPointer<TDC7200> tdc7200;
    bool spiDevicePresent = false;
    QPointer<TcpConnection> tcpConnection;
    QMap<uint16_t, int> msgRateCfgs;
    int waitingForAppliedMsgRate = 0;
    QPointer<QtSerialUblox> qtGps;
    QPointer<QTcpServer> tcpServer;
    QString peerAddress;
    QHostAddress daemonAddress = QHostAddress::Null;
    quint16 peerPort, daemonPort;
    QString gpsdevname;
    int verbose, baudrate;
    int gpsTimeout = 5000;
    bool dumpRaw, configGnss, showout, showin;
    bool mqttConnectionStatus = false;

    // file handling
    QPointer<FileHandler> fileHandler;

    // mqtt
    QPointer<MuonPi::MqttHandler> mqttHandler;

    // signal handling
    static int sighupFd[2];
    static int sigtermFd[2];
    static int sigintFd[2];

    QPointer<QSocketNotifier> snHup;
    QPointer<QSocketNotifier> snTerm;
    QPointer<QSocketNotifier> snInt;

    // calibration
    ShowerDetectorCalib* calib = nullptr;

    // histograms
    QMap<QString, Histogram> histoMap;

    // others
    QVector<QPointF> xorRatePoints, andRatePoints;
    timespec startOfProgram, lastRateInterval;
    quint32 rateBufferTime = 60; // in s: 60 seconds
    quint32 rateBufferInterval = 2000; // in ms: 2 seconds
    quint32 rateMaxShowInterval = 60 * 60 * 1000; // in ms: 1 hour
    QTimer rateBufferReminder;
    QTimer oledUpdateTimer;
    QList<quint64> andCounts, xorCounts;
    UbxDopStruct currentDOP;
    Property nrSats, nrVisibleSats, fixStatus;
    QVector<QTcpSocket*> peerList;
    QList<float> adcSamplesBuffer;
    ADC_SAMPLING_MODE adcSamplingMode = ADC_SAMPLING_MODE::PEAK;
    qint16 currentAdcSampleIndex = -1;
    QTimer samplingTimer;
    QTimer parameterMonitorTimer;
    QTimer rateScanTimer;
    QMap<QString, Property> propertyMap;
    LogEngine logEngine;

    // threads
    QPointer<QThread> mqttHandlerThread;
    QPointer<QThread> fileHandlerThread;
    QPointer<QThread> pigThread;
    QPointer<QThread> gpsThread;
    QPointer<QThread> tcpThread;

    configuration config;
	KalmanGnssFilter kalmanGnssFilter { 0.01 };
};

#endif // DAEMON_H
