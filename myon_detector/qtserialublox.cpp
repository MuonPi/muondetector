#include <QSerialPort>
#include "qtserialublox.h"

QtSerialUblox::QtSerialUblox(const QString &serialPortName, int baudRate,
                             QObject *parent, bool newDumpRaw, int newVerbose, QObject *parent) : QObject(parent)
{
    _portName = serialPortName;
    _baudRate = baudRate;
    verbose = newVerbose;
    dumpRaw = newDumpRaw;
}
