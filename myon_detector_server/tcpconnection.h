#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <QObject>
#include <QFile>
#include <QTcpSocket>
#include <time.h>
#include <QTimer>


class TcpConnection : public QObject
{
    Q_OBJECT

public:
    TcpConnection(int socketDescriptor, int verbose = 4, int timeout = 15000,
                  int pingInterval = 5000, QObject *parent = 0);
    void delay(int millisecondsWait);
    void startTimePulser();

signals:
    void stoppedConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort,
                           quint32 timeoutTime, quint32 connectionDuration);
    void madeConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort);
    void connectionTimeout(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort,
                           quint32 timeoutTime, quint32 connectionDuration);
    void error(int socketError, const QString &message);
    void toConsole(QString data);
    void stopTimePulser();

public slots:
    //void onSocketDisconnected();
    void doStuff();
    void onReadyRead();
    void onTimePulse();
    bool sendText(const quint16 someCode, QString someText);
    bool sendCode(const quint16 someCode);


private:
    bool handleFileTransfer(QString fileName, QByteArray &block, quint16 nextCount);
    int timeout;
    QDataStream *in;
    QByteArray carry;
    quint16 fileTransmissionCounter = -1;
    QFile *file = NULL;
    QHostAddress *peerAddress;
    quint16 peerPort;
    QHostAddress *localAddress;
    quint16 localPort;
    QTcpSocket *tcpSocket;
    QTimer *t;
    int socketDescriptor;
    int pingInterval;
    int verbose;
    time_t firstConnection;
    time_t lastConnection;
};

#endif // TCPCONNECTION_H
