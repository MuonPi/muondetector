#ifndef QTSERIALUBLOX_H
#define QTSERIALUBLOX_H

#include "structs_and_defines.h"
#include <QSerialPort>
#include <QObject>

class QtSerialUblox : public QObject
{
    Q_OBJECT

public:
    explicit QtSerialUblox(const QString serialPortName, int baudRate,
                           bool newDumpRaw, int newVerbose, QObject *parent = 0);

signals:
    void toConsole(QString data);
    void UBXCfgError(QString data);

public slots:
    void makeConnection();
    void onReadyRead();
    void handleError(QSerialPort::SerialPortError serialPortError);
    void UBXSetCfgMsg(uint16_t msgID, uint8_t port, uint8_t rate, int verbose);
    void UBXSetCfgMsg(uint8_t classID, uint8_t messageID, uint8_t port, uint8_t rate, int verbose);
    void calcChkSum(const std::string& buf, unsigned char* chkA, unsigned char* chkB);
    void UBXSetCfgRate(uint8_t measRate, uint8_t navRate, int verbose);


private:
    bool sendUBX(uint16_t msgID, unsigned char* payload, int nBytes);
    bool sendUBX(unsigned char classID, unsigned char messageID,
                                unsigned char* payload, int nBytes);
    QSerialPort *serialPort = nullptr;
    QString _portName;
    int _baudRate = 0;
    int verbose = 0;
    int timeout = 5000;
    bool dumpRaw = false;
};

#endif // QTSERIALUBLOX_H
