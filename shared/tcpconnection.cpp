#include <tcpconnection.h>
#include <QtNetwork>
#include <iostream>
#include <QDataStream>

const quint16 ping = 123;
const quint16 msgSig = 246;
const quint16 answPing = 231;
const quint16 fileSig = 142;
const quint16 nextPart = 143;
const quint16 quitConnection = 101;
const quint16 timeoutSig = 138;
const quint16 i2cProps = 275;
const quint16 i2cRequest = 271;
const quint16 ubxMsgRateRequest = 293;
const quint16 ubxMsgRate = 297;
const quint16 gpioPin = 331;
const quint16 msgCode = 333;

TcpConnection::TcpConnection(QString newHostName, quint16 newPort, int newVerbose, int newTimeout,
                             int newPingInterval, QObject *parent)
    : QObject(parent)
{
    verbose= newVerbose;
    hostName=newHostName;
    port=newPort;
    timeout=newTimeout;
    pingInterval = newPingInterval;
    //qRegisterMetaType<QVector<QVariant> >("QVector<QVariant>");
    qRegisterMetaType<TcpMessage> ("TcpMessage");
    qRegisterMetaType<I2cProperty> ("I2cProperty");
}

TcpConnection::~TcpConnection(){

      if(peerAddress!=nullptr){delete peerAddress;}
      if(localAddress!=nullptr){delete localAddress;}
      if(file!=nullptr){delete file;}
      if(in!=nullptr){delete in;}
      if(t!=nullptr){delete t;}
}

TcpConnection::TcpConnection(int socketDescriptor, int newVerbose, int newTimeout, int newPingInterval, QObject *parent)
    : QObject(parent), socketDescriptor(socketDescriptor)//, text(data)
{
    pingInterval = newPingInterval;
    timeout = newTimeout;
    verbose = newVerbose;
    //qRegisterMetaType<QVector<float> >("QVector<float>");
    qRegisterMetaType<TcpMessage> ("TcpMessage");
    qRegisterMetaType<I2cProperty> ("I2cProperty");
}

void TcpConnection::makeConnection()
// this function gets called with a signal from client-thread
// (TcpConnection runs in a separate thread only communicating with main thread through messages)
{
    if (verbose > 4){
        emit toConsole(QString("client tcpConnection running in thread " + QString("0x%1").arg((int)this->thread())));
    }
    tcpSocket = new QTcpSocket(this);
    in = new QDataStream();
    in->setVersion(QDataStream::Qt_4_0);
    in->setDevice(tcpSocket);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpConnection::onReadyRead);
    //connect(tcpSocket, &QTcpSocket::disconnected, this, &TcpConnection::disconnected);
    tcpSocket->connectToHost(hostName, port);
    firstConnection = time(NULL);
    lastConnection = firstConnection;
    if (!tcpSocket->waitForConnected(timeout)) {
        emit error(tcpSocket->error(), tcpSocket->errorString());
        this->thread()->quit();
        return;
    }
    emit connected();
    peerAddress = new QHostAddress(tcpSocket->peerAddress());
    peerPort = tcpSocket->peerPort();
    localAddress = new QHostAddress(tcpSocket->localAddress());
    localPort = tcpSocket->localPort();
    //startTimePulser();
}

void TcpConnection::receiveConnection()
{   // setting up tcpSocket.
    // only done once
    if (verbose > 4){
        emit toConsole(QString("client tcpConnection running in thread " + QString("0x%1").arg((int)this->thread())));
    }
    tcpSocket = new QTcpSocket(this);
    if (!tcpSocket->setSocketDescriptor(socketDescriptor)) {
        emit error(tcpSocket->error(),tcpSocket->errorString());
        this->thread()->quit();
        return;
    }else{
        peerAddress = new QHostAddress(tcpSocket->peerAddress());
        peerPort = tcpSocket->peerPort();
        localAddress = new QHostAddress(tcpSocket->localAddress());
        localPort = tcpSocket->localPort();
        lastConnection = time(NULL);
        //connect(tcpSocket, &QTcpSocket::disconnected, this, &TcpConnection::onSocketDisconnected);
        in = new QDataStream();
        in->setVersion(QDataStream::Qt_4_0);
        in->setDevice(tcpSocket);
        connect(tcpSocket, &QTcpSocket::readyRead, this, &TcpConnection::onReadyRead);
        firstConnection = time(NULL);
        lastConnection = firstConnection;
        emit madeConnection(peerAddress->toString(),peerPort,localAddress->toString(),localPort);
    }
    //startTimePulser();
}

void TcpConnection::closeConnection(){
    sendCode(quitConnection);
    this->thread()->quit();
    return;
}

void TcpConnection::onReadyRead(){
    // this function gets called when tcpSocket emits readyRead signal
    if(!in){
        emit toConsole("input stream not yet initialized");
        return;
    }
    if (blockSize==0){
        if (tcpSocket->bytesAvailable()<(int)(sizeof(quint16))){
            return;
        }
        *in >> blockSize;
    }
    if (tcpSocket->bytesAvailable()<blockSize){
        return;
    }
    if (tcpSocket->bytesAvailable()>blockSize){
        emit toConsole(QString(QString::number(tcpSocket->bytesAvailable()-blockSize)
                       +" more Bytes available than expected by blockSize"));
    }
    blockSize = 0;
    QByteArray block;
    *in >> block;
    TcpMessage tcpMessage(block);
    emit receivedTcpMessage(tcpMessage);
   // emit toConsole("something went wrong with the transmission code");
}

bool TcpConnection::sendTcpMessage(TcpMessage tcpMessage){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << tcpMessage.getData();
    out.device()->seek(0);
    out << (quint16)(block.size() - (int)sizeof(quint16));
    return writeBlock(block);
}

/*
bool TcpConnection::sendFile(QString fileName){
    if (!fileName.isEmpty()){
        if (file){
            file->close();
            delete file;
            file = nullptr;
        }
        file = new QFile(QCoreApplication::applicationDirPath()+"/"+fileName);
        if (!file->open(QIODevice::ReadOnly)) {
            emit toConsole("Could not open file, maybe no permission or file does not exist.");
            return false;
        }
        fileCounter = 0;
        QByteArray fileNameBlock;
        QDataStream out(&fileNameBlock, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_0);
        out << fileSig;
        out << fileCounter;
        out << fileName;
        tcpSocket->write(fileNameBlock);
        if(!tcpSocket->waitForBytesWritten(timeout)){
            return false;
        }
        return true;
    }
    if(file->atEnd()){
        emit toConsole("file at end");
        return true;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    fileCounter++;
    out << fileSig;
    out << fileCounter;
    out << file->read(100*1024);
    return writeBlock(block);
}

bool TcpConnection::sendI2CProperties(I2cProperty i2cProperty, bool setProperties){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << i2cProps;
    out << i2cProperty;
    out << setProperties;
    return writeBlock(block);
}

bool TcpConnection::sendI2CPropertiesRequest(){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << i2cRequest;
    return writeBlock(block);
}

bool TcpConnection::sendUbxMsgRatesRequest(){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << ubxMsgRateRequest;
    return writeBlock(block);
}

bool TcpConnection::sendGpioRisingEdge(quint8 pin, quint32 tick){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << gpioPin;
    out << pin;
    out << tick;
    return writeBlock(block);
}

bool TcpConnection::sendUbxMsgRates(QMap<uint16_t, int> msgRateCfgs){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << ubxMsgRate;
    out << msgRateCfgs;
    return writeBlock(block);
}

bool TcpConnection::handleFileTransfer(QString fileName, QByteArray &block, quint16 nextCount){
    if (nextCount == 0){
        fileCounter = 0;
        if(file){
            file->close();
            delete file;
            file = nullptr;
        }
        if (fileName.isEmpty()){
            emit toConsole("filename is empty, something went wrong");
            fileName = "./testfile.txt";
        }
        file = new QFile(QCoreApplication::applicationDirPath()+"/"+fileName);
        if(!file->open(QIODevice::ReadWrite)){
            if (verbose>2){
                emit toConsole("could not open "+fileName);
                return false;
            }
        }
        return true;
    }
    if (fileCounter+1!=nextCount){
        emit toConsole("received file transmission out of order");
        return false;
    }
    fileCounter = nextCount;
    file->write(block);
    return true;
}

bool TcpConnection::sendMessage(TcpMessage tcpMessage){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << msgCode;
    out << tcpMessage;
    return writeBlock(block);
}


bool TcpConnection::sendText(const quint16 someCode, QString someText){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << someCode;
    out << someText;
    // for qt version < 5.7:
    // send the size of the string so that receiver knows when
    // all data has been successfully received

    return writeBlock(block);
}

bool TcpConnection::sendCode(const quint16 someCode){
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << someCode;
    return writeBlock(block);
}
*/
bool TcpConnection::writeBlock(QByteArray &block){
    if (!tcpSocket) {
        emit toConsole("in client => tcpConnection:\ntcpSocket not instantiated");
        return false;
    }
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
    emit toConsole("tcp unconnected state before wait for bytes written, closing connection");
    this->thread()->quit();
    return false;
}
/*
void TcpConnection::onTimePulse(){
    if (fabs(time(NULL)-lastConnection)>timeout/1000){
        // hier einfügen, was passiert, wenn host nicht auf ping antwortet
        quint32 connectionDuration = (quint32)(time(NULL)-firstConnection);
        quint32 timeoutTime = (quint32)time(NULL);
        sendCode(timeoutSig);
        emit connectionTimeout(peerAddress->toString(),peerPort,localAddress->toString(),localPort,timeoutTime,connectionDuration);
        this->deleteLater();
        return;
    }
    if (tcpSocket){
        if (verbose>3){
            emit toConsole("sending ping");
        }
        sendCode(ping);
    }
}

void TcpConnection::startTimePulser()
{
    t = new QTimer(this);
    t->setInterval(pingInterval);
    connect(this, &TcpConnection::stopTimePulser, t, &QTimer::stop);
    connect(t, &QTimer::timeout, this, &TcpConnection::onTimePulse);
    t->start();
}
*/
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
