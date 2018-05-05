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
    void delay(int millisecondsWait);
    void startTimePulser();

signals:
    //void posixTerminate();
    void error(int socketError, QString message);
    void connectionTimeout(QString hostName, quint16 port, quint32 timeoutTime, quint32 connectionDuration);
    void toConsole(QString data);

public slots:
    void makeConnection();
    void closeConnection();
    //void disconnected();
    //void onPosixTerminate();
    void onReadyRead();
    void onTimePulse();
    bool sendFile(QString fileName = "");
    bool sendCode(const quint16 someCode);
    bool sendText(const quint16 someCode, QString someText);

private:
    int timeout;
    int verbose;
    int pingInterval;
    quint16 fileCounter = 0;
    QFile *myFile = nullptr;
    QDataStream *in = nullptr;
    QTcpSocket *tcpSocket = nullptr;
	QString hostName;
	quint16 port;
    QTimer *t = nullptr;
    time_t lastConnection;
    time_t firstConnection;
};

#endif // TCPCONNECTION_H
