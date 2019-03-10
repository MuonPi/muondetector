#ifndef DAEMON_H
#define DAEMON_H

#include <QObject>
#include <QPointer>
#include <QTcpServer>
#include <wiringPi.h>
#include <tcpconnection.h>
#include <custom_io_operators.h>
#include <qtserialublox.h>
#include <QSocketNotifier>
#include <pigpiodhandler.h>
#include <filehandler.h>
#include "i2c/i2cdevices.h"
#include "calibration.h"
#include <logparameter.h>
#include <histogram.h>

// for sig handling:
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

class Daemon : public QTcpServer
{
	Q_OBJECT

public:
    Daemon(QString username, QString password, QString new_gpsdevname, int new_verbose, quint8 new_pcaPortMask,
        float *new_dacThresh, float new_biasVoltage, bool bias_ON, bool new_dumpRaw, int new_baudrate,
        bool new_configGnss, QString new_PeerAddress, quint16 new_PpeerPort,
        QString new_serverAddress, quint16 new_serverPort, bool new_showout, bool new_showin, QObject *parent = 0);
	~Daemon();
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
    void connectToGps();
    void connectToServer();
    void displaySocketError(int socketError, QString message);
    void displayError(QString message);
    void toConsole(const QString& data);
    void gpsToConsole(const QString& data);
    void stoppedConnection(QString hostName, quint16 port, quint32 connectionTimeout, quint32 connectionDuration);
    void UBXReceivedAckNak(uint16_t ackedMsgID, uint16_t ackedCfgMsgID);
    void UBXReceivedMsgRateCfg(uint16_t msgID, uint8_t rate);
    void gpsConnectionError();
    void gpsPropertyUpdatedInt32(int32_t data, std::chrono::duration<double> updateAge,
		char propertyName);
    void gpsPropertyUpdatedUint32(uint32_t data, std::chrono::duration<double> updateAge,
		char propertyName);
    void gpsPropertyUpdatedUint8(uint8_t data, std::chrono::duration<double> updateAge,
		char propertyName);
    void gpsPropertyUpdatedGnss(std::vector<GnssSatellite>,
        std::chrono::duration<double> updateAge);
    void onUBXReceivedGnssConfig(uint8_t numTrkCh, const std::vector<GnssConfigStruct>& gnssConfigs);
    void onUBXReceivedTP5(const UbxTimePulseStruct& tp);
    void gpsMonHWUpdated(uint16_t noise, uint16_t agc, uint8_t antStatus, uint8_t antPower, uint8_t jamInd, uint8_t flags);
    void gpsMonHW2Updated(int8_t ofsI, uint8_t magI, int8_t ofsQ, uint8_t magQ, uint8_t cfgSrc);
    void receivedTcpMessage(TcpMessage tcpMessage);
    void pollAllUbxMsgRate();
    void sendGpioPinEvent(uint8_t gpio_pin);
    void sendUbxGeodeticPos(GeodeticPos pos);
    void UBXReceivedVersion(const QString& swString, const QString& hwString, const QString& protString);
    void sampleAdc0Event();
    void sampleAdcEvent(uint8_t channel);
    void getTemperature();
    void scanI2cBus();
    void onUBXReceivedTimeTM2(timespec rising, timespec falling, uint32_t accEst, bool valid, uint8_t timeBase, bool utcAvailable);
	
signals:
    void sendTcpMessage(TcpMessage tcpMessage);
    void closeConnection(QString closeAddress);
    void logParameter(LogParameter log);
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
    void UBXSetDynModel(uint8_t model);
    void resetUbxDevice(uint32_t flags);
    void setGnssConfig(const std::vector<GnssConfigStruct>& gnssConfigs);
    void UBXSetMinMaxSVs(uint8_t minSVs, uint8_t maxSVs);
    void UBXSetMinCNO(uint8_t minCNO);
    void GpioSetInput(unsigned int gpio);
    void GpioSetOutput(unsigned int gpio);
    void GpioSetPullUp(unsigned int gpio);
    void GpioSetPullDown(unsigned int gpio);
    void GpioSetState(unsigned int gpio, bool state);
    void UBXSetCfgTP5(const UbxTimePulseStruct& tp);
    void UBXSetAopCfg(bool enable=true, uint16_t maxOrbErr=0);
	
private:
    void incomingConnection(qintptr socketDescriptor) override;
    void setPcaChannel(uint8_t channel); // channel 0 to 3
											 // 0: coincidence ; 1: xor ; 2: discr 1 ; 3: discr 2
    void sendPcaChannel();
    void setDacThresh(uint8_t channel, float threshold); // channel 0 or 1 ; threshold in volts
    void sendDacThresh(uint8_t channel);
    void sendDacReadbackValue(uint8_t channel, float voltage);
    void setBiasVoltage(float voltage);
    void sendBiasVoltage();
    void sendBiasStatus();
    void sendGainSwitchStatus();
    void setBiasStatus(bool status);
    void sendPreampStatus(uint8_t channel);
	void setUbxMsgRates(QMap<uint16_t,int>& ubxMsgRates);
    void sendUbxMsgRates();
    void sendGpioRates(int number = 0, quint8 whichRate = 0);
    void sendI2cStats();
    void sendCalib();
    void sendHistogram(const Histogram& hist);
    bool readEeprom();
    void receivedCalibItems(const std::vector<CalibStruct>& newCalibs);
    void logBiasValues();
    void setupHistos();
    void rescaleHisto(Histogram& hist, double center, double width);
    void rescaleHisto(Histogram& hist, double center);
    void checkRescaleHisto(Histogram& hist, double newValue);
    
    void printTimestamp();
    void delay(int millisecondsWait);
    MCP4728* dac = nullptr;
    ADS1115* adc = nullptr;
    PCA9536* pca = nullptr;
    LM75* lm75 = nullptr;
    EEPROM24AA02* eep = nullptr;
    UbloxI2c* ubloxI2c = nullptr;
    float biasVoltage = 0.;
    bool biasON = false;
    bool gainSwitch = false;
    bool preampStatus[2];
    uint8_t pcaPortMask = 0;
    QVector <float> dacThresh; // do not give values here because of push_back in constructor of deamon
    QPointer<PigpiodHandler> pigHandler;
    QPointer<TcpConnection> tcpConnection;
	QMap <uint16_t, int> msgRateCfgs;
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

    // file handling
    QPointer<FileHandler> fileHandler;

    // signal handling
	static int sighupFd[2];
	static int sigtermFd[2];
	static int sigintFd[2];

    QPointer<QSocketNotifier> snHup;
    QPointer<QSocketNotifier> snTerm;
    QPointer<QSocketNotifier> snInt;
    
    ShowerDetectorCalib* calib = nullptr;
    
    Histogram geoHeightHisto, geoLonHisto, geoLatHisto,
     weightedGeoHeightHisto,
     pulseHeightHisto, adcSampleTimeHisto,
     ubxTimeLengthHisto, eventIntervalHisto, eventIntervalShortHisto, 
     ubxTimeIntervalHisto, tpTimeDiffHisto;
    UbxDopStruct currentDOP;
    timespec lastTimestamp = { 0, 0 };
};

#endif // DAEMON_H
