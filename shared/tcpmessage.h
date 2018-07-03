#ifndef TCPMESSAGE_H
#define TCPMESSAGE_H
#include <QObject>
//#include <QByteArray>
#include <QVector>

//class  MessageCoder : public QObject{
//    Q_OBJECT
//public:
//MessageCoder(QObject* parent = 0);

//private:
//QByteArray block;
//};

//union MessageContent{

//};
union MyUnion{
public:
    int i;
    bool b;
    float f;
};

struct MessageContent{
    unsigned int type;
    MyUnion myUnion;
    friend QDataStream& operator<<(QDataStream& in, MessageContent& content);
    friend QDataStream& operator>>(QDataStream& out, MessageContent& content);
};
class TcpMessage{
public:
    QVector<int> information;
    QVector<MessageContent> data;
    friend QDataStream& operator<<(QDataStream& in, TcpMessage& message);
    friend QDataStream& operator>>(QDataStream& out, TcpMessage& message);
};


#endif // TCPMESSAGE_H
