#include "qtserialublox.h"
#include <sstream>
//#include "custom_io_operators.h" // remove after debug
using namespace std;

QtSerialUblox::QtSerialUblox(const QString serialPortName, int baudRate,
                             bool newDumpRaw, int newVerbose, bool newShowout, QObject *parent) : QObject(parent)
{
    _portName = serialPortName;
    _baudRate = baudRate;
    verbose = newVerbose;
    dumpRaw = newDumpRaw;
    showout = newShowout;
}

void QtSerialUblox::makeConnection(){
    // this function gets called with a signal from client-thread
    // (QtSerialUblox runs in a separate thread only communicating with main thread through messages)
    if (verbose > 4){
        emit toConsoleNewLine(QString("gps running in thread " + QString( "0x%1" ).arg( (int)this->thread())));
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
    serialPort->clear(QSerialPort::AllDirections);
}

void QtSerialUblox::onReadyRead(){
    // this function gets called when the serial port emits readyRead signal
    //cout << "\nstart readyread" <<endl;
    QByteArray temp = serialPort->readAll();
    if (dumpRaw) {
        emit toConsole(QString(temp));
        // if put std::string to console it gets stuck!?
        //emit stdToConsole(temp.toStdString());
    }
    buffer += temp.toStdString();
    //cout << QString(temp).toStdString();
    UbxMessage message;
    if(scanUnknownMessage(buffer, message)){
        // so it found a message therefore we can now process the message
        processMessage(message);
    }
}

bool QtSerialUblox::scanUnknownMessage(string &buffer, UbxMessage &message)
{   // gets the (maybe not finished) buffer and checks for messages in it that make sense
    if (buffer.size() < 9) return false;
    std::string refstr = "";
    // refstr are the first two hex numbers defining the header of an ubx message
    refstr += 0xb5; refstr += 0x62;
    std::string::size_type found = buffer.find(refstr);
    std::string mess = "";
    if (found != string::npos)
    {
        mess = buffer.substr(found, buffer.size());
        // discard everything before start of the message
        buffer.erase(0, found);
    }
    else{
        // discard everything before the start of a NMEA message, too
        // to ensure that buffer won't grow too big
        if(discardAllNMEA){
            std::string beginNMEA = "$";
            size_t found = 0;
            while (found!= string::npos){
                found = buffer.find(beginNMEA);
                if (found== string::npos){
                    break;
                }
                buffer.erase(0,found+1);
            }
        }
        return false;
    }
    message.classID = (uint8_t)mess[2];
    message.messageID = (uint8_t)mess[3];
    message.msgID = (uint16_t)mess[2] << 8;
    message.msgID += (uint16_t)mess[3];
    if (mess.size() < 8) return false;
    //        cout<<"received UBX string: ";
    //        for (int i=0; i<mess.size(); i++) cout<<hex<<(int)mess[i]<<" ";
    //        cout<<endl;
    int len = (int)(mess[4]);
    len += ((int)(mess[5])) << 8;
    if (((long int)mess.size() - 8) < len) {
        std::string::size_type found = buffer.find(refstr, 2);
        if (found != string::npos) {
            std::stringstream tempStream;
            tempStream << "received faulty UBX string:\n " << dec;
            for (std::string::size_type i = 0; i < found; i++) tempStream << hex << (int)buffer[i] << " ";
            emit toConsole(QString::fromStdString(tempStream.str()));
            buffer.erase(0, found);
        }
        return false;
    }
    //    cout<<"len="<<dec<<len<<endl;
    //    cout<<"mess.size()="<<dec<<mess.size()<<endl;
    //	cout<<"da2"<<endl;
    buffer.erase(0, len + 8);

    unsigned char chkA;
    unsigned char chkB;
    calcChkSum(mess.substr(0, len + 6), &chkA, &chkB);
    //     cout<<"chkA/B="<<hex<<(int)chkA<<" "<<(int)chkB<<endl;
    //     cout<<"mess[len+6/7]="<<hex<<(int)mess[len+6]<<" "<<(int)mess[len+7]<<endl;
    if (mess[len + 6] == chkA && mess[len + 7] == chkB)
    {
        message.data = mess.substr(6, len);
        return true;
    }

    return false;
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
    //QByteArray block;
    //block.append(QString::fromStdString(s));
    if (serialPort){
        if (serialPort->write(s.c_str())){
            if(serialPort->waitForBytesWritten(timeout)){
                if (showout){
                    std::stringstream tempStream;
                    tempStream << "\nmessage sent: ";
                    for (std::string::size_type i = 0; i < s.length(); i++){
                        tempStream << "0x"<<std::setfill('0') << std::setw(2) << std::hex << (int)s[i] << " ";
                    }
                    tempStream << "\n";
                    emit toConsole(QString::fromStdString(tempStream.str()));
                }
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

void QtSerialUblox::UBXSetCfgRate(uint8_t measRate, uint8_t navRate)
{
    if (verbose>4){
        emit toConsoleNewLine(QString("Ublox UBXsetCfgRate running in thread "  + QString( "0x%1" )
                               .arg( (int)this->thread())));
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

void QtSerialUblox::UBXSetCfgMsg(uint16_t msgID, uint8_t port, uint8_t rate)
{
    if (verbose>4){
        emit toConsoleNewLine(QString("Ublox UBXsetCfgMsg running in thread "  + QString( "0x%1" )
                               .arg( (int)this->thread())));
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

void QtSerialUblox::UBXSetCfgMsg2(uint8_t classID, uint8_t messageID, uint8_t port, uint8_t rate)
{
    uint16_t msgID = (messageID + (uint16_t)classID) << 8;
    UBXSetCfgMsg(msgID, port, rate);
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



bool QtSerialUblox::pollUBX(uint16_t msgID, std::string& answer, int timeout)
{
    sendUBX(msgID, NULL, 0);
    int iter = 0;
    answer = "";
    //std::string buffer = "";
    while (iter < timeout && !answer.size())
    {
        /*
                std::string buf="";
                int n=ReadBuffer(buf);
                if (n>0) {
                    buffer+=buf;
                    iter++;
                    answer=scanMessage(buffer,classID,messageID);
                }
        */
        // include something about sleep maybe 1 ms
        //usleep(1);
        iter++;
        //mutex.lock();
        for (std::list<UbxMessage>::iterator it = fMessageBuffer.begin(); it != fMessageBuffer.end(); it++) {
            if (it->msgID == msgID) {
                answer = it->data;
                fMessageBuffer.erase(it);
                break;
            }
        }
        //mutex.unlock();
    }
    if (iter >= timeout) return false;
    if (verbose > 2) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "received UBX string: ";
        for (std::string::size_type i = 0; i < answer.size(); i++) tempStream << hex << (int)answer[i] << " ";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    if (answer.size()) return true;
    return false;
}

bool QtSerialUblox::pollUBX(uint8_t classID, uint8_t messageID, std::string& answer, int timeout)
{
    uint16_t msgID = (messageID + (uint16_t)classID) << 8;
    return pollUBX(msgID, answer, timeout);
}
