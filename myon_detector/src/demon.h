#ifndef DEMON_H
#define DEMON_H

#include <QObject>
#include <QTcpServer>
#include <wiringPi.h>
#include <tcpconnection.h>
#include <custom_io_operators.h>
#include <qtserialublox.h>
#include <i2c/i2cdevices.h>
#include <tcpmessage.h>

class Demon : public QTcpServer
{
	Q_OBJECT

public:
    Demon(QString new_gpsdevname, int new_verbose, quint8 new_pcaChannel,
        float *new_dacThresh, float new_biasVoltage,bool biasPower, bool new_dumpRaw, int new_baudrate,
        bool new_configGnss, QString new_PeerAddress, quint16 new_PpeerPort,
        QString new_serverAddress, quint16 new_serverPort, bool new_showout, QObject *parent = 0);
    ~Demon();
	void configGps();
	void loop();

public slots:
    void connectToGps();
    void connectToServer();
    void displaySocketError(int socketError, QString message);
	void displayError(QString message);
    void toConsole(QString data);
    void gpsToConsole(QString data);
    void stoppedConnection(QString hostName, quint16 port, quint32 connectionTimeout, quint32 connectionDuration);
    void UBXReceivedAckAckNak(bool ackAck, uint16_t ackedMsgID, uint16_t ackedCfgMsgID);
    void gpsConnectionError();
    void gpsPropertyUpdatedInt32(int32_t data, std::chrono::duration<double> updateAge,
                            char propertyName);
    void gpsPropertyUpdatedUint32(uint32_t data, std::chrono::duration<double> updateAge,
                            char propertyName);
    void gpsPropertyUpdatedUint8(uint8_t data, std::chrono::duration<double> updateAge,
                            char propertyName);
    void gpsPropertyUpdatedGnss(std::vector<GnssSatellite>,
                            std::chrono::duration<double> updateAge);
    void sendI2CProperties();
    void setI2CProperties(I2cProperty i2cProperty, bool setProperties);

signals:
    void sendFile(QString fileName);
    void sendMsg(QString msg);
    void sendMessage(TcpMessage tcpMessage);
    void closeConnection();
    void sendPoll(uint16_t msgID, uint8_t port);
    void i2CProperties(I2cProperty i2cProperty, bool set_Properties = false);
	void UBXSetCfgMsg(uint16_t msgID, uint8_t port, uint8_t rate);
	void UBXSetCfgRate(uint8_t measRate, uint8_t navRate);
    void UBXSetCfgPrt(uint8_t gpsPort, uint8_t outProtocolMask);

private:
    void incomingConnection(qintptr socketDescriptor) override;
    void pcaSelectTimeMeas(uint8_t channel); // channel 0 to 3
                                             // 0: coincidence ; 1: xor ; 2: discr 1 ; 3: discr 2
    void dacSetThreashold(uint8_t channel, float threashold); // channel 0 or 1 ; threashold in volts
    MCP4728 *dac;
    QVector <float> dacThresh;
    float biasVoltage;
    bool biasPowerOn = false;
    ADS1115 *adc;
    PCA9536 *pca;
    int pcaChannel;
    LM75 *lm75;
    TcpConnection * tcpConnection = nullptr;
    QMap <uint16_t, int> messageConfiguration;
    QtSerialUblox *qtGps = nullptr;
    QString peerAddress;
    QHostAddress demonAddress = QHostAddress::Null;
    quint16 peerPort, demonPort;
    QTcpServer *tcpServer;
	void printTimestamp();
	void delay(int millisecondsWait);
    QString gpsdevname;
    int verbose, baudrate;
    int gpsTimeout = 5000;
    bool dumpRaw, configGnss, showout;
};

#endif // DEMON_H
