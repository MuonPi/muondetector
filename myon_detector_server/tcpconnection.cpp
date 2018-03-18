#include "tcpconnection.h"
#include <QtNetwork>

const quint8 ping = 123;
const quint8 msgSig = 246;
const quint8 answPing = 231;
const quint8 fileSig = 142;
const quint8 quitConnection = 101;
const quint8 timeoutSig = 138;

TcpConnection::TcpConnection(int socketDescriptor, int newVerbose, int newTimeout, int newPingInterval, QObject *parent)
    : QObject(parent), socketDescriptor(socketDescriptor)//, text(data)
{
    pingInterval = newPingInterval;
    timeout = newTimeout;
    verbose = newVerbose;
}

void TcpConnection::doStuff()
{   // setting up tcpSocket.
    // only done once
    tcpSocket = new QTcpSocket();
    if (!tcpSocket->setSocketDescriptor(socketDescriptor)) {
        emit error(tcpSocket->error(),tcpSocket->errorString());
        return;
    }else{
        peerAddress = new QHostAddress(tcpSocket->peerAddress());
        peerPort = tcpSocket->peerPort();
        localAddress = new QHostAddress(tcpSocket->localAddress());
        localPort = tcpSocket->localPort();
        lastConnection = time(NULL);
        //connect(tcpSocket, &QTcpSocket::disconnected, this, &TcpConnection::onSocketDisconnected);
        in = new QDataStream();
        in->setDevice(tcpSocket);
        connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpConnection::onReadyRead);
        firstConnection = time(NULL);
        lastConnection = firstConnection;
        emit madeConnection(peerAddress->toString(),peerPort,localAddress->toString(),localPort);
    }
    //startTimePulser();
}

void TcpConnection::onReadyRead(){
    // what happens when a packet from the client arrives
    if(!in){ return; }
    QByteArray block;
    quint8 someCode;
    QString someData;
    QDataStream incomingData(&block,QIODevice::ReadWrite);
    in->startTransaction();
    *in >> block;
    if (!in->commitTransaction()){
        return;
    }
    //incomingData >> someCode;
    // cout << block.toStdString()<<endl;
    if (someCode == fileSig){
        emit toConsole("receiving data file");
        lastConnection = time(NULL);
        if (!handleFileTransfer(incomingData)){
            // eventually send some information to server that transmission failed
        }
        return;
    }
    if (someCode == msgSig){
        emit toConsole("received message");
        QString temp;
        incomingData >> temp;
        emit toConsole(someData);
        return;
    }
    if (someCode == ping){
        if (verbose>3){
            emit toConsole("received ping, sending answerping");
        }
        sendData(answPing, "");
        lastConnection = time(NULL);
        return;
    }
    if (someCode == answPing){
        if(verbose>3){
            emit toConsole("received answerping");
        }
        lastConnection = time(NULL);
        return;
    }
    if (someCode == quitConnection){
        this->deleteLater();
        return;
    }
    if (someCode == timeoutSig){
        quint32 connectionDuration = (quint32)(time(NULL)-firstConnection);
        emit connectionTimeout(peerAddress->toString(),peerPort,localAddress->toString(),localPort,(quint32)time(NULL),connectionDuration);
        return;
    }
}

bool TcpConnection::handleFileTransfer(QDataStream& incomingData){
    quint16 nextCount;
    incomingData >> nextCount;
    if (nextCount == 0){
        fileTransmissionCounter = nextCount;
        if(file){
            file->close();
            delete file;
        }
        QString fileName;
        incomingData >> fileName;
        if (fileName.isEmpty()){
            emit toConsole("filename is empty, something went wrong");
            fileName = "./testfile.txt";
        }
        file = new QFile(fileName);
        if(!file->open(QIODevice::WriteOnly)){
            if (verbose>2){
                emit toConsole("could not open "+fileName);
                return false;
            }
        }
    }
    if (fileTransmissionCounter+1!=nextCount){
        return false;
    }
    fileTransmissionCounter = nextCount;
    QByteArray temp;
    incomingData >> temp;
    QDataStream toFile(file);
    toFile << temp;
    return true;
}

bool TcpConnection::sendData(const quint8 someCode, QString someData){
    if(!tcpSocket){
        emit toConsole("in client => tcpConnection:\ntcpSocket not instantiated");
        return false;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    // for qt version < 5.7:
    // out << (quint16)0;
    out << someCode;
    out << someData;
    // for qt version < 5.7:
    // send the size of the string so that receiver knows when
    // all data has been successfully received
    /*out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    */
    tcpSocket->write(block);
    for(int i = 0; i<3; i++){
        if(!tcpSocket->state()==QTcpSocket::UnconnectedState){
            if(!tcpSocket->waitForBytesWritten(timeout)){
                emit toConsole("wait for bytes written timeout");
                return false;
            }
            return true;
        }else{
            delay(100);
        }
    }
    emit toConsole("tcp unconnected state before wait for bytes written");
    return false;
}

void TcpConnection::onTimePulse(){
    if (fabs(time(NULL)-lastConnection)>timeout/1000){
        // hier einfügen, was passiert, wenn host nicht auf ping antwortet
        quint32 connectionDuration = (quint32)(time(NULL)-firstConnection);
        quint32 timeoutTime = (quint32)time(NULL);
        sendData(timeoutSig,"");
        emit connectionTimeout(peerAddress->toString(),peerPort,localAddress->toString(),localPort,timeoutTime,connectionDuration);
        this->deleteLater();
        return;
    }
    if (tcpSocket/*&&tcpSocket->state()!=QTcpSocket::UnconnectedState*/){
        if (verbose>3){
            emit toConsole("sending ping");
        }
        sendData(ping, "");
    }
}

/*void TcpConnection::onSocketDisconnected(){
    emit connectionTimeout(peerAddress->toString(), peerPort, localAddress->toString(), localPort, (quint32)time(NULL), (quint32)(time(NULL)-firstConnection));
}*/

void TcpConnection::startTimePulser()
{
    t = new QTimer();
    t->setInterval(pingInterval);
    connect(this, &TcpConnection::stopTimePulser, t, &QTimer::stop);
    connect(t, &QTimer::timeout, this, &TcpConnection::onTimePulse);
    t->start();
}

void TcpConnection::delay(int millisecondsWait)
{
    QEventLoop loop;
    QTimer delay;
    //connect(this, &TcpConnection::posixTerminate, &loop, &QEventLoop::quit);
    //connect(tcpSocket, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
    delay.connect(&delay, &QTimer::timeout, &loop, &QEventLoop::quit);
    delay.start(millisecondsWait);
    loop.exec();
}
