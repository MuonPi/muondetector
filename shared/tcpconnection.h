#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <QTcpSocket>
#include <time.h>
#include <QTimer>
#include <QFile>

class TcpConnection : public QObject
{
	Q_OBJECT

public:
    TcpConnection(QString hostName, quint16 port, int verbose = 0, int timeout = 15000,
                  int pingInterval = 5000, QObject *parent = 0);
    TcpConnection(int socketDescriptor, int verbose = 0, int timeout = 15000,
                  int pingInterval = 5000, QObject *parent = 0);
    ~TcpConnection();
    void delay(int millisecondsWait);
    void startTimePulser();

signals:
    void stoppedConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort,
                           quint32 timeoutTime, quint32 connectionDuration);
    void madeConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort);
    void connectionTimeout(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort,
                           quint32 timeoutTime, quint32 connectionDuration);
    void error(int socketError, const QString message);
    void toConsole(QString data);
    void stopTimePulser();
    void connected();
    void i2CProperties(quint8 pcaChann, QVector<float> dac_Thresh,
                                                float bias_Voltage,
                                                bool bias_powerOn,
                                                bool set_Properties);
    void requestI2CProperties();

public slots:
    void makeConnection();
    void receiveConnection();
    void closeConnection();
    void onReadyRead();
    void onTimePulse();
    bool sendFile(QString fileName = "");
    bool sendText(const quint16 someCode, QString someText);
    bool sendCode(const quint16 someCode);
    bool sendI2CProperties(quint8 pcaChann, QVector<float> dac_Thresh,
                             float bias_Voltage,
                             bool bias_powerOn, bool setProperties = false);
    bool sendI2CPropertiesRequest();

private:
    void handleI2CProperties(QByteArray &block);
    bool handleFileTransfer(QString fileName, QByteArray &block, quint16 nextCount);
    bool writeBlock(QByteArray &block);
    int timeout;
    int verbose;
    int pingInterval;
    int socketDescriptor;
    QHostAddress *peerAddress;
    QHostAddress *localAddress;
    quint16 fileCounter = -1;
    QFile *file = nullptr;
    QDataStream *in = nullptr;
    QTcpSocket *tcpSocket = nullptr;
	QString hostName;
    quint16 peerPort;
    quint16 localPort;
    QTimer *t = nullptr;
    time_t lastConnection;
    time_t firstConnection;
    quint16 port = 0;
};

#endif // TCPCONNECTION_H
