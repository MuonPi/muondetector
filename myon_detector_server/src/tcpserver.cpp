#include <QThread>
#include <QNetworkInterface>
#include "tcpserver.h"
#include "custom_io_operators.h"

using namespace std;

TcpServer::TcpServer(QString listenIpAddress, quint16 portFromStart ,int newVerbose, QObject *parent)
    : QTcpServer(parent)
{
    verbose = newVerbose;
    if (!listenIpAddress.isEmpty()){
    // use ipAddress from start parameters or not
        if (listenIpAddress == "localhost" || listenIpAddress == "local"){
            ipAddress = QHostAddress::LocalHost;
        }else{
            ipAddress = QHostAddress(listenIpAddress);
        }
    }else{
        ipAddress = QHostAddress(QHostAddress::Any);
    }
    // it is possible to select specific port from star parameters
    port = portFromStart;
    if (port == 0){
        // maybe think about other fall back solution
        port = 51508;
    }
    if (!this->listen(ipAddress,port)) {
        cout << tr("Unable to start the server: %1.\n").arg(this->errorString());
    }else{
        cout <<tr("\nThe server is running on\n\nIP: %1\nport: %2\n\n")
                             .arg(ipAddress.toString()).arg(serverPort());
    }
    flush(cout);
}

// specify what happens when a client makes first contact with the server
void TcpServer::incomingConnection(qintptr socketDescriptor)
{
    QThread *thread = new QThread();
    TcpConnection *tcpConnection = new TcpConnection(socketDescriptor,verbose);
    tcpConnection->moveToThread(thread);
    connect(thread, SIGNAL(started()), tcpConnection, SLOT(receiveConnection()));
    connect(thread, SIGNAL(finished()), tcpConnection, SLOT(deleteLater()));
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    //QObject::connect(thread, &QThread::started, &tcpConnection, &TcpConnection::doStuff);
    //QObject::connect(thread, &QThread::finished, &tcpConnection, &TcpConnection::deleteLater);
    QObject::connect(tcpConnection, &TcpConnection::madeConnection, this, &TcpServer::madeConnection);
    QObject::connect(tcpConnection, &TcpConnection::stoppedConnection, this, &TcpServer::stoppedConnection);
    QObject::connect(tcpConnection, &TcpConnection::connectionTimeout, this, &TcpServer::connectionTimeout);
    QObject::connect(tcpConnection, &TcpConnection::toConsole, this, &TcpServer::toConsole);
    thread->start();
}
// slots to make use of the signals, emitted from threads. Only way to communicate to the main thread
void TcpServer::madeConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress,quint16 localPort)
{
    cout<<tr("server: made connection with %1:%2 on %3:%4  now handled by thread\n")
                 .arg(remotePeerAddress).arg(remotePeerPort).arg(localAddress).arg(localPort)<<endl;
}

void TcpServer::stoppedConnection(QString remotePeerAddress, quint16 remotePeerPort,
                                  QString localAddress, quint16 localPort, qint32 connectionTimeout, qint32 connectionDuration)
{
    cout<<tr("\nserver: stopped connection with %1:%2 on %3:%4")
                 .arg(remotePeerAddress).arg(remotePeerPort).arg(localAddress).arg(localPort)<<endl;
    cout<<"connection timeout at "<<connectionTimeout<<"  connection lasted "<<connectionDuration<<"s"<<endl;
}
void TcpServer::connectionTimeout(QString remotePeerAddress, quint16 remotePeerPort,
                                  QString localAddress, quint16 localPort, qint32 connectionTimeout, qint32 connectionDuration){
    cout<<tr("\nserver: stopped connection with %1:%2 on %3:%4")
                 .arg(remotePeerAddress).arg(remotePeerPort).arg(localAddress).arg(localPort)<<endl;
    cout<<"connection timeout at "<<connectionTimeout<<"  connection lasted "<<connectionDuration<<"s"<<endl;
}


void TcpServer::toConsole(QString data){
    cout<< "server: "<<data<<endl;
}

void TcpServer::displayError(int socketError, const QString &message)
{
    switch (socketError) {
    case QAbstractSocket::HostNotFoundError:
        cout <<tr("The host was not found. Please check the "
                      "host and port settings.\n");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        cout <<tr("The connection was refused by the peer. "
                      "Make sure the fortune server is running, "
                      "and check that the host name and port "
                      "settings are correct.\n");
        flush(cout);
        break;
    default:
        cout <<tr("The following error occurred: %1.\n").arg(message);
    }
}

