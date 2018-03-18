#include "tcpconnection.h"
#include <QtNetwork>
#include <sstream>

const quint8 ping = 123;
const quint8 msgSig = 246;
const quint8 answPing = 231;
const quint8 fileSig = 142;
const quint8 quitConnection = 101;
const quint8 timeoutSig = 138;

TcpConnection::TcpConnection(QString newHostName, quint16 newPort, int newVerbose, int newTimeout,
                             int newPingInterval, QObject *parent)
    : QObject(parent)
{
    verbose= newVerbose;
    hostName=newHostName;
    port=newPort;
    timeout=newTimeout;
    pingInterval = newPingInterval;
}

void TcpConnection::makeConnection()
{
    std::stringstream str;
    str << "client tcpConnection reporting from thread " << this->thread();
    emit toConsole(QString::fromStdString(str.str()));
    tcpSocket = new QTcpSocket();
    in = new QDataStream();
    in->setDevice(tcpSocket);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpConnection::onReadyRead);
    //connect(tcpSocket, &QTcpSocket::disconnected, this, &TcpConnection::disconnected);
    tcpSocket->connectToHost(hostName, port);
    firstConnection = time(NULL);
    lastConnection = firstConnection;
    if (!tcpSocket->waitForConnected(timeout)) {
        emit error(tcpSocket->error(), tcpSocket->errorString());
        return;
    }
    //startTimePulser();
}

void TcpConnection::closeConnection(){
    sendData(quitConnection,"");
    this->deleteLater();
    return;
}

void TcpConnection::onReadyRead(){
    if(!in){ return; }
    quint8 someCode;
    QByteArray block;
    in->startTransaction();
    *in >> block;
    //*in >> data;
    if (!in->commitTransaction()){
        return;
    }

    // do something with data
    if (someCode == ping){
        if (verbose>3){
            emit toConsole("received ping, sending answerping");
        }
        sendData(answPing, "");
        lastConnection = time(NULL);
    }
    if (someCode == answPing){
        if(verbose>3){
            emit toConsole("received answerping");
        }
        lastConnection = time(NULL);
    }
    if (someCode == timeoutSig){
        emit connectionTimeout(hostName,port,(quint32)time(NULL),(quint32)(time(NULL)-firstConnection));
    }
}

/*
void TcpConnection::onPosixTerminate(){
    emit posixTerminate();
}
*/

bool TcpConnection::sendData(const quint8 someCode, QString someData){
    if (!tcpSocket) {
        emit toConsole("in client => tcpConnection:\ntcpSocket not instantiated");
        return false;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
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

bool TcpConnection::sendMsg(QString message){
    if(!sendData(msgSig,message)){
        emit toConsole("unable to send message");
        return false;
    }
    return true;
}

bool TcpConnection::sendFile(QString fileName){
    if (!tcpSocket) {
        emit toConsole("in client => tcpConnection:\ntcpSocket not instantiated");
        return false;
    }
    QFile myFile(fileName);
    if (!myFile.open(QIODevice::ReadOnly)) {
        emit toConsole("Could not open file, maybe no permission or file does not exist.");
        return false;
    }
    QByteArray block;
    //quint16 fileCounter = 0;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << fileSig;
    //out << fileCounter;
    //fileCounter++;
    out << fileName;
    tcpSocket->write(block);
    if(!tcpSocket->waitForBytesWritten(timeout)){
        return false;
    }
    while (true){
        block.clear();
        //out << fileSig;
        //out << fileCounter;
        //fileCounter++;
        //out << myFile.readLine(150);
        out << fileSig;
        block.append(myFile.read(8*32768));
        tcpSocket->write(block);
        if(/*tcpSocket->state()==QTcpSocket::UnconnectedState||*/!tcpSocket->waitForBytesWritten(timeout)){
            emit toConsole("wait for bytes written timeout while sending file");
            return false;
        }
    }
    return true;
}

void TcpConnection::onTimePulse(){
    if (fabs(time(NULL)-lastConnection)>timeout/1000){

        // hier einfügen, was passiert, wenn host nicht auf ping antwortet
        quint32 timeoutTime = (quint32)time(NULL);
        quint32 connectionDuration = (quint32)(time(NULL)-firstConnection);
        emit connectionTimeout(hostName, port, timeoutTime, connectionDuration);
        emit toConsole("until deletelater");
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


void TcpConnection::startTimePulser()
{
    t = new QTimer();
    t->setInterval(pingInterval);
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

//void disconnected(){}
