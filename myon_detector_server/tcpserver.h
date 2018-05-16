#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QStringList>
#include <QTcpServer>
#include <QFile>
#include "../shared/tcpconnection.h"

class TcpServer : public QTcpServer
{
    Q_OBJECT

public:
    TcpServer(QString listenIpAddress, quint16 portFromStart , int verbose = 0, QObject *parent = 0);

private slots:
    void incomingConnection(qintptr socketDescriptor) override;
    void madeConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort);
    void stoppedConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort,
                           qint32 connectionTimeout, qint32 connectionDuration);
    void connectionTimeout(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort,
                           qint32 connectionTimeout, qint32 connectionDuration);
    void displayError(int socketError, const QString &message);
    void toConsole(QString data);

private:
    QStringList someRandomText;
    QHostAddress ipAddress = QHostAddress::Null;
    quint16 port;
    int verbose;
};
#endif // TCPSERVER.H
