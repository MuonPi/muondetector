#ifndef DAEMON_H
#define DAEMON_H

#include <QObject>
#include <QTcpServer>
#include <wiringPi.h>
#include <tcpconnection.h>
#include <custom_io_operators.h>
#include <qtserialublox.h>
#include <i2c/i2cdevices.h>
#include <QSocketNotifier>

// for sig handling:
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>

class Daemon : public QTcpServer
{
	Q_OBJECT

public:
    Daemon(QString new_gpsdevname, int new_verbose, quint8 new_pcaPortMask,
        float *new_dacThresh, float new_biasVoltage, bool bias_ON, bool new_dumpRaw, int new_baudrate,
        bool new_configGnss, QString new_PeerAddress, quint16 new_PpeerPort,
        QString new_serverAddress, quint16 new_serverPort, bool new_showout, bool new_showin, QObject *parent = 0);
	~Daemon();
	void configGps();
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
	void toConsole(QString data);
	void gpsToConsole(QString data);
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
	void receivedTcpMessage(TcpMessage tcpMessage);
    void pollAllUbxMsgRate();
    void sendGpioPinEvent(uint8_t gpio_pin);

signals:
	void sendTcpMessage(TcpMessage tcpMessage);
	void aboutToQuit();
	void sendPollUbxMsgRate(uint16_t msgID);
	void sendPollUbxMsg(uint16_t msgID);
	// difference between msgRate and msg is that CFG-MSG (0x0601) alone is used
	// to set/get the rate for every message so the msgID must be wrapped to the data
    // of a message of type CFG-MSG first
	void UBXSetCfgMsgRate(uint16_t msgID, uint8_t port, uint8_t rate);
	void UBXSetCfgRate(uint8_t measRate, uint8_t navRate);
	void UBXSetCfgPrt(uint8_t gpsPort, uint8_t outProtocolMask);

private:
	void incomingConnection(qintptr socketDescriptor) override;
    void setPcaChannel(uint8_t channel); // channel 0 to 3
											 // 0: coincidence ; 1: xor ; 2: discr 1 ; 3: discr 2
    void sendPcaChannel();
    void setDacThresh(uint8_t channel, float threshold); // channel 0 or 1 ; threshold in volts
    void sendDacThresh(uint8_t channel);
    void setBiasVoltage(float voltage);
    void sendBiasVoltage();
    void sendBiasStatus();
    void setBiasStatus(bool status);
    void sendUbxMsgRates();
    void printTimestamp();
    void delay(int millisecondsWait);
    MCP4728 *dac = nullptr;
    ADS1115 *adc = nullptr;
    PCA9536 *pca = nullptr;
    LM75 *lm75 = nullptr;
    float biasVoltage = 0;
    bool biasON = false;
    uint8_t pcaPortMask = 0;
    QVector <float> dacThresh; // do not give values here because of push_back in constructor of deamon
	TcpConnection * tcpConnection = nullptr;
	QMap <uint16_t, int> msgRateCfgs;
	QtSerialUblox *qtGps = nullptr;
    QTcpServer *tcpServer = nullptr;
	QString peerAddress;
	QHostAddress daemonAddress = QHostAddress::Null;
    quint16 peerPort, daemonPort;
	QString gpsdevname;
	int verbose, baudrate;
	int gpsTimeout = 5000;
	bool dumpRaw, configGnss, showout, showin;

	// signal handling
	static int sighupFd[2];
	static int sigtermFd[2];
	static int sigintFd[2];

	QSocketNotifier *snHup;
	QSocketNotifier *snTerm;
	QSocketNotifier *snInt;
};

#endif // DAEMON_H
