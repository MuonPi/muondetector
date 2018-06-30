#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <QTcpSocket>
#include <time.h>
#include <QTimer>
#include <QFile>
#include <i2cproperty.h>

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
                           quint32 timeoutTime = 0, quint32 connectionDuration = 0);
    void madeConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort);
    void connectionTimeout(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort,
                           quint32 timeoutTime, quint32 connectionDuration);
    void error(int socketError, const QString message);
    void toConsole(QString data);
    void stopTimePulser();
    void connected();
    void i2CProperties(I2cProperty i2cProperty, bool setProperties);
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
    bool sendI2CProperties(I2cProperty i2cProperty, bool setProperties);
    bool sendI2CPropertiesRequest();

private:
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
