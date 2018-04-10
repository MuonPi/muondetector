#include "qtserialublox.h"
#include "custom_io_operators.h" // remove after debug
using namespace std; //remove after debug

QtSerialUblox::QtSerialUblox(const QString serialPortName, int baudRate,
                             bool newDumpRaw, int newVerbose, QObject *parent) : QObject(parent)
{
    _portName = serialPortName;
    _baudRate = baudRate;
    verbose = newVerbose;
    dumpRaw = newDumpRaw;
}

void QtSerialUblox::makeConnection(){
    // this function gets called with a signal from client-thread
    // (QtSerialUblox runs in a separate thread only communicating with main thread through messages)
    if (verbose > 2){
        emit toConsole(QString("gps running in thread " + QString( "0x%1" ).arg( (int)this->thread(), 16 )));
    }
    if (serialPort){
        delete(serialPort);
    }
    serialPort = new QSerialPort(_portName);
    serialPort->setBaudRate(_baudRate);
    if (!serialPort->open(QIODevice::ReadWrite)) {
        emit toConsole(QObject::tr("Failed to open port %1, error: %2")
                          .arg(_portName)
                          .arg(serialPort->errorString()));
        return;
    }
    connect(serialPort, &QSerialPort::readyRead, this, &QtSerialUblox::onReadyRead);
}

void QtSerialUblox::onReadyRead(){
    // this function gets called when the serial port emits readyRead signal
    QByteArray temp = serialPort->readAll();
    emit toConsole(QString(temp));
}

void QtSerialUblox::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError) {
        emit toConsole(QObject::tr("An I/O error occurred while reading "
                                        "the data from port %1, error: %2")
                            .arg(serialPort->portName())
                            .arg(serialPort->errorString()));
    }
}
