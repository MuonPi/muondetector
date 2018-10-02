#include <tcpconnection.h>
#include <QtNetwork>
#include <iostream>
#include <QDataStream>

TcpConnection::TcpConnection(QString newHostName, quint16 newPort, int newVerbose, int newTimeout,
	int newPingInterval, QObject *parent)
	: QObject(parent)
{
	verbose = newVerbose;
	hostName = newHostName;
	port = newPort;
	timeout = newTimeout;
	pingInterval = newPingInterval;
	//qRegisterMetaType<QVector<QVariant> >("QVector<QVariant>");
}

TcpConnection::TcpConnection(int socketDescriptor, int newVerbose, int newTimeout, int newPingInterval, QObject *parent)
	: QObject(parent), socketDescriptor(socketDescriptor)//, text(data)
{
	pingInterval = newPingInterval;
	timeout = newTimeout;
	verbose = newVerbose;
	//qRegisterMetaType<QVector<float> >("QVector<float>");
}

TcpConnection::~TcpConnection()
{
    if (peerAddress != nullptr) { delete peerAddress; }
    if (localAddress != nullptr) { delete localAddress; }
    if (in != nullptr) { delete in; }
    if (t != nullptr) { delete t; }
}

void TcpConnection::makeConnection()
// this function gets called with a signal from client-thread
// (TcpConnection runs in a separate thread only communicating with main thread through messages)
{
	if (verbose > 4) {
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
	if (verbose > 4) {
		emit toConsole(QString("client tcpConnection running in thread " + QString("0x%1").arg((int)this->thread())));
	}
	tcpSocket = new QTcpSocket(this);
	if (!tcpSocket->setSocketDescriptor(socketDescriptor)) {
		emit error(tcpSocket->error(), tcpSocket->errorString());
		this->thread()->quit();
		return;
	}
	else {
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
		emit madeConnection(peerAddress->toString(), peerPort, localAddress->toString(), localPort);
	}
	//startTimePulser();
}

void TcpConnection::closeConnection() {
	//sendCode(quitConnection);
	this->thread()->quit();
	return;
}

void TcpConnection::onReadyRead() {
	// this function gets called when tcpSocket emits readyRead signal
	if (!in) {
		emit toConsole("input stream not yet initialized");
		return;
	}
	if (blockSize == 0) {
		if (tcpSocket->bytesAvailable() < (int)(sizeof(quint16))) {
			return;
		}
        *in >> blockSize;
	}
	if (tcpSocket->bytesAvailable() < blockSize) {
		return;
	}
    if (tcpSocket->bytesAvailable() > blockSize) {
		emit toConsole(QString(QString::number(tcpSocket->bytesAvailable() - blockSize)
			+ " more Bytes available than expected by blockSize"));
    }
    QByteArray block;
    char data[blockSize];
    in->readRawData(data,blockSize);
    block.setRawData(data,blockSize);
    blockSize = 0;
    //*in >> block;
    qDebug() << "Bytes read:";
    qDebug() << block.size();
    qDebug() << block;
	TcpMessage tcpMessage(block);
	emit receivedTcpMessage(tcpMessage);
	// emit toConsole("something went wrong with the transmission code");
}

bool TcpConnection::sendTcpMessage(TcpMessage tcpMessage) {
    QByteArray block;
    block = tcpMessage.getData();
    QDataStream stream(&block, QIODevice::ReadWrite);
    stream.device()->seek(0);
    stream << (quint16)(block.size() - (int)sizeof(quint16)); // size of payload
    return writeBlock(block);
}

bool TcpConnection::writeBlock(const QByteArray &block) {
	if (!tcpSocket) {
		emit toConsole("in client => tcpConnection:\ntcpSocket not instantiated");
		return false;
	}
	tcpSocket->write(block);
	for (int i = 0; i < 3; i++) {
		if (!tcpSocket->state() == QTcpSocket::UnconnectedState) {
			if (!tcpSocket->waitForBytesWritten(timeout)) {
				emit toConsole("wait for bytes written timeout");
				return false;
			}
			return true;
		}
		else {
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
