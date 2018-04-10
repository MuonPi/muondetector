#ifndef QTSERIALUBLOX_H
#define QTSERIALUBLOX_H

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


public slots:
    void makeConnection();
    void onReadyRead();
    void handleError(QSerialPort::SerialPortError serialPortError);

private:
    QSerialPort *serialPort = nullptr;
    QString _portName;
    int _baudRate = 0;
    int verbose = 0;
    bool dumpRaw = false;
};

#endif // QTSERIALUBLOX_H
