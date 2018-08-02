#include "tcpmessage.h"
#include <QDebug>

TcpMessage::TcpMessage(quint16 tcpMsgID)
{
	msgID = tcpMsgID;
	dStream = new QDataStream(&data, QIODevice::ReadWrite);
	*dStream << tcpMsgID;
}

TcpMessage::TcpMessage(QByteArray& rawdata) {
	data = rawdata;
	//    quint64 pos = dStream->device()->pos();
	dStream = new QDataStream(&data, QIODevice::ReadWrite);
	//    if (!dStream->device()->seek(0)){
	//        qDebug() << "failed to seek position " << 0 << " in dStream";
	//    }
	//    *dStream >> msgID;
	//    if(!dStream->device()->seek(pos)){
	//        qDebug() << "failed to seek position " << pos << " in dStream";
	//    }
	*dStream >> msgID;
}


//TcpMessage::~TcpMessage() {
//    if (dStream!=nullptr) {
//        delete dStream;
//        dStream = nullptr;
//    }
//}

QByteArray TcpMessage::getData() {
	return data;
}

void TcpMessage::setData(QByteArray& rawData) {
	data = rawData;
}

void TcpMessage::setMsgID(quint16 tcpMsgID) {
	msgID = tcpMsgID;
}

quint16 TcpMessage::getMsgID() {
	return msgID;
}
