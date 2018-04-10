#ifndef QTSERIALUBLOX_H
#define QTSERIALUBLOX_H

#include <QObject>

class QtSerialUblox : public QObject
{
    Q_OBJECT

public:
    explicit QtSerialUblox(const QString &serialPortName = nullptr, int baudRate, QObject *parent,
                           bool newDumpRaw, int newVerbose, QObject *parent);

signals:

public slots:
private:
    QString _portName;
    int _baudRate = 0;
    int verbose = 0;
    bool dumpRaw = false;
};

#endif // QTSERIALUBLOX_H
