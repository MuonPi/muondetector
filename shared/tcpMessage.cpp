#include "tcpmessage.h"

TcpMessage::TcpMessage(quint16 tcpMsgID)
{
    msgID = tcpMsgID;
    dStream = new QDataStream(&data, QIODevice::ReadWrite);
    dStream << tcpMsgID;
}

TcpMessage::TcpMessage(QByteArray& rawdata){
    data = rawdata;
    quint64 pos = dStream->device()->pos();
    dStream = new QDataStream(data, QIODevice::ReadWrite);
    if (!dStream->device()->seek(0)){
        return false;
    }
    dStream >> msgID;
    if(!dStream->device()->seek(pos)){
        return false;
    }
    return true;
}


TcpMessage::~TcpMessage(){
    delete dStream;
}

TcpMessage::getData(){
    return data;
}

TcpMessage::setData(QByteArray& rawData){
    data = rawData;
}

TcpMessage::setMsgID(quint16 tcpMsgID){
    msgID = tcpMsgID;
}

TcpMessage::tcpMsgID(){
    return msgID;
}
