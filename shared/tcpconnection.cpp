#include "tcpconnection.h"
#include <QtNetwork>

const quint16 ping = 123;
const quint16 msgSig = 246;
const quint16 answPing = 231;
const quint16 fileSig = 142;
const quint16 nextPart = 143;
const quint16 quitConnection = 101;
const quint16 timeoutSig = 138;

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

TcpConnection::TcpConnection(int socketDescriptor, int newVerbose, int newTimeout, int newPingInterval, QObject *parent)
    : QObject(parent), socketDescriptor(socketDescriptor)//, text(data)
{
    pingInterval = newPingInterval;
    timeout = newTimeout;
    verbose = newVerbose;
}

void TcpConnection::makeConnection()
// this function gets called with a signal from client-thread
// (TcpConnection runs in a separate thread only communicating with main thread through messages)
{
    if (verbose > 4){
        emit toConsole(QString("client tcpConnection running in thread " + QString("0x%1").arg((int)this->thread())));
    }
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
    peerAddress = new QHostAddress(tcpSocket->peerAddress());
    peerPort = tcpSocket->peerPort();
    localAddress = new QHostAddress(tcpSocket->localAddress());
    localPort = tcpSocket->localPort();
    //startTimePulser();
}

void TcpConnection::receiveConnection()
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

void TcpConnection::closeConnection(){
    sendCode(quitConnection);
    this->deleteLater();
    return;
}

void TcpConnection::onReadyRead(){
    // this function gets called when tcpSocket emits readyRead signal
    if(!in){
        emit toConsole("input stream not yet initialized");
        return;
    }
    QByteArray block;
    quint16 someCode = 0;
    quint16 nextCount = -1;
    QString someMsg;
    QString fileName;
    // first get some code (defined at the top of this file)
    // to see what kind of message this is
    in->startTransaction();
    *in >> someCode;

    // if the message belongs to a file transfer:
    if (someCode == fileSig){
        *in >> nextCount;
        if (nextCount == 0){
            *in >> fileName;
        }else{
            *in >> block;
        }
    }

    // if the message is just some command (consisting of one string):
    if (someCode == msgSig){
        *in >> someMsg;
    }

    // if the code says "nextPart" it means: send the next part of the file transmission!
    if (someCode == nextPart){
        // really important even if you put "" in the block it must be read out here or it will hang...
        // you have to get EVERYTHING out of the DataStream or it won't work!!!
        *in >> someMsg;
        lastConnection = time(NULL);
        sendFile();
    }

    // if this is not a complete transaction but just a part -> return
    if (!in->commitTransaction()){
        return;
    }

    // if it's a file transfer the message is stored in "block" now so we can handle it
    if (someCode == fileSig){
        lastConnection = time(NULL);
        if (!handleFileTransfer(fileName, block, nextCount)){
            emit toConsole("handle file transfer failed");
            // eventually send some information to server that transmission failed
        }
        sendCode(nextPart);
        return;
    }

    // if code says something else:
    if (someCode == ping){
        if (verbose>3){
            emit toConsole("received ping, sending answerping");
        }
        sendCode(answPing);
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
    emit toConsole("something went wrong with the transmission code");
}

bool TcpConnection::sendFile(QString fileName){
    if (!tcpSocket) {
        emit toConsole("in client => tcpConnection:\ntcpSocket not instantiated");
        return false;
    }
    if (!fileName.isEmpty()){
        if (file){
            file->close();
            delete file;
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
    tcpSocket->write(block);
    if(!tcpSocket->waitForBytesWritten(timeout)){
        emit toConsole("wait for bytes written timeout while sending file");
        return false;
    }
    return true;
}

bool TcpConnection::handleFileTransfer(QString fileName, QByteArray &block, quint16 nextCount){
    if (nextCount == 0){
        fileCounter = 0;
        if(file){
            file->close();
            delete file;
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

bool TcpConnection::sendText(const quint16 someCode, QString someText){
    if (!tcpSocket) {
        emit toConsole("in client => tcpConnection:\ntcpSocket not instantiated");
        return false;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << someCode;
    out << someText;
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

bool TcpConnection::sendCode(const quint16 someCode){
    if (!tcpSocket) {
        emit toConsole("in client => tcpConnection:\ntcpSocket not instantiated");
        return false;
    }
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_0);
    out << someCode;
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
        sendCode(timeoutSig);
        emit connectionTimeout(peerAddress->toString(),peerPort,localAddress->toString(),localPort,timeoutTime,connectionDuration);
        this->deleteLater();
        return;
    }
    if (tcpSocket/*&&tcpSocket->state()!=QTcpSocket::UnconnectedState*/){
        if (verbose>3){
            emit toConsole("sending ping");
        }
        sendCode(ping);
    }
}

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
