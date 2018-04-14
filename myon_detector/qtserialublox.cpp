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

bool QtSerialUblox::sendUBX(uint16_t msgID, unsigned char* payload, int nBytes)
{
    std::string s = "";
    s += 0xb5; s += 0x62;
    s += (unsigned char)((msgID & 0xff00) >> 8);
    s += (unsigned char)(msgID & 0xff);
    s += (unsigned char)(nBytes & 0xff);
    s += (unsigned char)((nBytes & 0xff00) >> 8);
    for (int i = 0; i < nBytes; i++) s += payload[i];
    unsigned char chkA, chkB;
    calcChkSum(s, &chkA, &chkB);
    s += chkA;
    s += chkB;
    //   cout<<endl<<endl;
    //   cout<<"write UBX string: ";
    //   for (int i=0; i<s.size(); i++) cout<<hex<<(int)s[i]<<" ";
    //   cout<<endl<<endl;
    //if (s.size() == WriteBuffer(s)) return true;
    QByteArray block;
    block.append(QString::fromStdString(s));
    if (serialPort){
        if (serialPort->write(block)){
            if(serialPort->waitForBytesWritten(timeout)){
               return true;
            }else{
                emit toConsole("wait for bytes written timeout while trying to write to serialPort");
            }
        }else{
            emit toConsole("error writing to serialPort");
        }
    }else{
        emit toConsole("error: serialPort not instantiated");
    }
    return false;
}

bool QtSerialUblox::sendUBX(unsigned char classID, unsigned char messageID, unsigned char* payload, int nBytes)
{
    uint16_t msgID = (messageID + (uint16_t)classID) << 8;
    return sendUBX(msgID, payload, nBytes);
}

void QtSerialUblox::calcChkSum(const std::string& buf, unsigned char* chkA, unsigned char* chkB)
{
    // calc Fletcher checksum, ignore the message header (b5 62)
    *chkA = 0;
    *chkB = 0;
    for (std::string::size_type i = 2; i < buf.size(); i++)
    {
        *chkA += buf[i];
        *chkB += *chkA;
    }
}

void QtSerialUblox::UBXSetCfgRate(uint8_t measRate, uint8_t navRate, int verbose)
{
    if (verbose>2){
        emit toConsole(QString("Ublox UBXsetCfgRate running in thread "  + QString( "0x%1" )
                               .arg( (int)this->thread(), 16 )));
    }
        unsigned char data[6];
    if (measRate < 10 || navRate < 1 || navRate>127) {
        emit UBXCfgError("measRate <10 || navRate<1 || navRate>127 error");
    }
    data[0] = measRate & 0xff;
    data[1] = (measRate & 0xff00) >> 8;
    data[2] = navRate & 0xff;
    data[3] = (navRate & 0xff00) >> 8;
    data[4] = 0;
    data[5] = 0;

    sendUBX(MSG_CFG_RATE, data, sizeof(data));
    /* Ack check has to be done differently, asynchronously
    if (waitAck(MSG_CFG_RATE, 10000))
    {
        emit toConsole("Set CFG successful");
    }
    else {
        emit UBXCfgError("Set CFG timeout");
    }
    */
}

void QtSerialUblox::UBXSetCfgMsg(uint16_t msgID, uint8_t port, uint8_t rate, int verbose)
{
    if (verbose>2){
        emit toConsole(QString("Ublox UBXsetCfgMsg running in thread "  + QString( "0x%1" )
                               .arg( (int)this->thread(), 16 )));
    }
    unsigned char data[8];
    if (port > 5) {
        emit UBXCfgError("port > 5 is not possible");
    }
    data[0] = (uint8_t)((msgID & 0xff00) >> 8);
    data[1] = msgID & 0xff;
    //   cout<<"data[0]="<<(int)data[0]<<" data[1]="<<(int)data[1]<<endl;
    data[2 + port] = rate;
    sendUBX(MSG_CFG_MSG, data, 8);
    /* Ack check has to be done differently, asynchronously
    if (waitAck(MSG_CFG_MSG, 12000)) {
        emit toConsole("Set CFG successful");
    }
    else {
        emit UBXCfgError("Set CFG timeout");
    }
    */
}

void QtSerialUblox::UBXSetCfgMsg(uint8_t classID, uint8_t messageID, uint8_t port, uint8_t rate, int verbose)
{
    uint16_t msgID = (messageID + (uint16_t)classID) << 8;
    UBXSetCfgMsg(msgID, port, rate, verbose);
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
