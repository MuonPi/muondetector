#include <tcpmessage_old.h>
#include <QDataStream>

//MessageCoder::MessageCoder(): QObject(parent){

//}
QDataStream& operator<<(QDataStream& in, const MessageContent& content){
    in << content.type;
    switch(content.type){
    case 0: in << content.myUnion.i;
        break;
    case 1: in << content.myUnion.f;
        break;
    case 2: in << content.myUnion.b;
        break;
    default:
        break;
    }
    return in;
}

QDataStream& operator>>(QDataStream& out, MessageContent& content){
    out >> content.type;
    switch(content.type){
    case 0: out >> content.myUnion.i;
        break;
    case 1: out >> content.myUnion.f;
        break;
    case 2: out >> content.myUnion.b;
        break;
    default:
        break;
    }
    return out;
}

QDataStream& operator<<(QDataStream& in, const TcpMessage& message) {
    in << message.data;
    //in << message.data;
    return in;
}

QDataStream& operator<<(QDataStream& in, TcpMessage& message) {
    in <<  message.data;
    // in << message.data;
    // ausgabe, welche funktion aufgerufen wird hinzufuegen
    return in;
}

QDataStream& operator>>(QDataStream& out, TcpMessage& message) {
    out >> message.data;
    //out >> message.data;
    return out;
}
