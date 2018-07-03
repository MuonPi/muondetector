#ifndef MESSAGECODER_H
#define MESSAGECODER_H
#include <QObject>
#include <QByteArray>
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
struct TcpMessage{
    QVector<int> information;
    QVector<QVariant<int, float, bool> > data;
};


#endif // MESSAGECODER_H
