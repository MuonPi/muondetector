#include "qtserialublox.h"
#include <QDebug>
#include <QEventLoop>
#include <QThread>
#include <iomanip>
#include <iostream>
#include <muondetector_structs.h>
#include <sstream>
#include <sys/syscall.h>
#include <ublox_messages.h>
#include <unistd.h>

constexpr std::size_t MAX_SEND_RETRIES { 5 };

std::string QtSerialUblox::fProtVersionString = "";

void QtSerialUblox::closeAll()
{
    if ((serialPort != nullptr) && (serialPort->isOpen())) {
        serialPort->flush();
        serialPort->close();
    }
}

std::string QtSerialUblox::toStdString(unsigned char* data, int dataSize)
{
    std::stringstream tempStream;
    for (int i = 0; i < dataSize; i++) {
        tempStream << data[i];
    }
    return tempStream.str();
}

QtSerialUblox::QtSerialUblox(const QString serialPortName, int newTimeout, int baudRate,
    bool newDumpRaw, int newVerbose, bool newShowout, bool newShowin, QObject* parent)
    : QObject(parent)
{
    _portName = serialPortName;
    _baudRate = baudRate;
    verbose = newVerbose;
    dumpRaw = newDumpRaw;
    showout = newShowout;
    showin = newShowin;
    timeout = newTimeout;
}

void QtSerialUblox::makeConnection()
{
    // this function gets called with a signal from client-thread
    // (QtSerialUblox runs in a separate thread only communicating with main thread through messages)
    if (verbose > 4) {
        qInfo() << this->thread()->objectName() << " thread id (pid): " << syscall(SYS_gettid);
    }
    if (!serialPort.isNull()) {
        serialPort.clear();
    }
    serialPort = new QSerialPort(_portName);
    serialPort->setBaudRate(_baudRate);
    if (!serialPort->open(QIODevice::ReadWrite)) {
        emit gpsConnectionError();
        emit toConsole(QObject::tr("Failed to open port %1, error: %2\ntrying again in %3 seconds\n")
                           .arg(_portName)
                           .arg(serialPort->errorString())
                           .arg(timeout));
        serialPort.clear();
        delay(timeout);
        emit gpsRestart();
        this->deleteLater();
        return;
    }
    ackTimer = new QTimer();
    ackTimer->setSingleShot(true);
    connect(ackTimer, &QTimer::timeout, this, &QtSerialUblox::ackTimeout);
    connect(serialPort, &QSerialPort::readyRead, this, &QtSerialUblox::onReadyRead);
    serialPort->clear(QSerialPort::AllDirections);
    if (verbose > 2) {
        emit toConsole("rising               falling               accEst valid timebase utc\n");
    }
}

void QtSerialUblox::sendQueuedMsg(bool afterTimeout)
{
    if (afterTimeout) {
        if (!msgWaitingForAck) {
            return;
        }
        if (++sendRetryCounter >= MAX_SEND_RETRIES) {
            sendRetryCounter = 0;
            ackTimer->stop();
            msgWaitingForAck.reset(nullptr);
            if (verbose > 2)
                emit toConsole("sendQueuedMsg: deleted message after 5 timeouts\n");
            sendQueuedMsg();
            return;
        }
        ackTimer->start(timeout);
        sendUBX(*msgWaitingForAck);
        if (verbose > 2)
            emit toConsole("sendQueuedMsg: repeated resend after timeout\n");
        return;
    }
    if (outMsgBuffer.empty()) {
        return;
    }
    if (msgWaitingForAck) {
        if (verbose > 2) {
            emit toConsole("tried to send queued message but ack for previous message not yet received\n");
        }
        return;
    }
    msgWaitingForAck = std::make_unique<UbxMessage>(outMsgBuffer.front());
    outMsgBuffer.pop();
    ackTimer->start(timeout);
    sendUBX(*msgWaitingForAck);
    if (verbose > 3)
        emit toConsole("sendQueuedMsg: sent fresh message\n");
}

void QtSerialUblox::ackTimeout()
{
    if (!msgWaitingForAck) {
        return;
    }
    if (verbose > 2) {
        std::stringstream tempStream;
        tempStream << "ack timeout, trying to resend message 0x" << std::setfill('0') << std::setw(2) << std::hex
                   << static_cast<int>(msgWaitingForAck->class_id()) << " 0x" << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(msgWaitingForAck->message_id());
        for (unsigned int i = 0; i < msgWaitingForAck->payload().size(); i++) {
            tempStream << " 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)(msgWaitingForAck->payload()[i]);
        }
        tempStream << std::endl;
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    sendQueuedMsg(true);
}

void QtSerialUblox::onReadyRead()
{
    // this function gets called when the serial port emits readyRead signal
    if (serialPort.isNull()) {
        return;
    }
    QByteArray temp = serialPort->readAll();
    if (dumpRaw) {
        emit toConsole(QString(temp));
    }
    for (int i = 0; i < temp.size(); i++) {
        m_buffer += temp.data()[i];
    }
    UbxMessage message;
    while (scanUnknownMessage(m_buffer, message)) {
        // so it found a message therefore we can now process the message
        if (showin) {
            std::stringstream tempStream {};
            tempStream << " in: ";
            int classID = message.class_id();
            int msgID = message.message_id();
            tempStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << classID;
            tempStream << " 0x" << std::setfill('0') << std::setw(2) << std::hex << msgID << " ";
            for (std::string::size_type i = 0; i < message.payload().size(); i++) {
                tempStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << (int)(message.payload()[i]) << " ";
            }
            tempStream << "\n";
            emit toConsole(QString::fromStdString(tempStream.str()));
        }
        processMessage(message);
    }
}

bool QtSerialUblox::scanUnknownMessage(std::string& buffer, UbxMessage& message)
{ // gets the (maybe not finished) buffer and checks for messages in it that make sense
    if (buffer.size() < 9)
        return false;
    // refstr are the first two hex numbers defining the header of an ubx message
    const std::string refstr { 0xb5, 0x62 };
    std::size_t found = buffer.find(refstr);
    if (found == std::string::npos) {
        // discard everything before the start of a NMEA message, too
        // to ensure that buffer won't grow too big
        if (discardAllNMEA) {
            std::string beginNMEA = "$";
            found = 0;
            while (found != std::string::npos) {
                found = buffer.find(beginNMEA);
                if (found == std::string::npos) {
                    break;
                }
                buffer.erase(0, found + 1);
            }
        }
        return false;
    }

    const std::string message_raw_data { buffer.substr(found, buffer.size()) };
    // discard everything before start of the message
    buffer.erase(0, found);

    if (message_raw_data.size() < 8)
        return false;
    int len = (int)(message_raw_data[4]);
    len += ((int)(message_raw_data[5])) << 8;
    if (((long int)message_raw_data.size() - 8) < len) {
        found = buffer.find(refstr, 2);
        if (found != std::string::npos) {
            if (verbose > 1) {
                std::stringstream tempStream;
                tempStream << "received faulty UBX string:\n " << std::dec;
                for (std::string::size_type i = 0; i < found; i++)
                    tempStream
                        << std::setw(2) << std::hex << "0x" << (int)buffer[i] << " ";
                tempStream << std::endl;
                emit toConsole(QString::fromStdString(tempStream.str()));
            }
            buffer.erase(0, found);
        }
        return false;
    }
    buffer.erase(0, len + 8);

    std::uint16_t msg_id { static_cast<std::uint16_t>((message_raw_data[2] << 8) | (message_raw_data[3] & 0xff)) };

    UbxMessage temp_message { msg_id, message_raw_data.substr(6, len) };
    auto checksum { temp_message.check_sum() };
    if ((checksum & 0xff) == message_raw_data[len + 6]
        && (checksum >> 8) == message_raw_data[len + 7]) {
        message = std::move(temp_message);
        return true;
    }
    return false;
}

bool QtSerialUblox::sendUBX(uint16_t msgID, const std::string& payload, uint16_t nBytes)
{
    const UbxMessage message { msgID, payload };
    return sendUBX(message);
}

bool QtSerialUblox::sendUBX(uint16_t msgID, unsigned char* payload, uint16_t nBytes)
{
    std::string payload_string { reinterpret_cast<const char*>(payload), nBytes };
    const UbxMessage message { msgID, payload_string };
    return sendUBX(message);
}

bool QtSerialUblox::sendUBX(const UbxMessage& msg)
{
    std::string raw_data_string { msg.raw_message_string() };
    if (!serialPort.isNull()) {
        QByteArray block(raw_data_string.c_str(), raw_data_string.size());
        if (serialPort->write(block)) {
            if (serialPort->waitForBytesWritten(timeout)) {
                if (showout) {
                    std::stringstream tempStream {};
                    tempStream << "out: ";
                    for (std::string::size_type i = 2; i < raw_data_string.length(); i++) {
                        tempStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << (int)raw_data_string[i] << " ";
                    }
                    tempStream << "\n";
                    emit toConsole(QString::fromStdString(tempStream.str()));
                }
                return true;
            } else {
                emit toConsole("wait for bytes written timeout while trying to write to serialPort");
            }
        } else {
            emit toConsole("error writing to serialPort");
        }
    } else {
        emit toConsole("error: serialPort not instantiated");
    }
    return false;
}

void QtSerialUblox::UBXSetCfgRate(uint16_t measRate, uint16_t navRate)
{
    if (verbose > 3) {
        emit toConsole(QString("Ublox UBXsetCfgRate running in thread " + QString("0x%1\n").arg((int)syscall(SYS_gettid))));
    }
    unsigned char data[6];
    // initialise all contents from data as 0 first!
    for (int i = 0; i < 6; i++) {
        data[i] = 0;
    }
    if (measRate < 10 || navRate < 1 || navRate > 127) {
        emit UBXCfgError("measRate <10 || navRate<1 || navRate>127 error");
    }
    data[0] = measRate & 0xff;
    data[1] = (measRate & 0xff00) >> 8;
    data[2] = navRate & 0xff;
    data[3] = (navRate & 0xff00) >> 8;
    data[4] = 0;
    data[5] = 0;

    enqueueMsg(UBX_MSG::CFG_RATE, toStdString(data, 6));
}

void QtSerialUblox::UBXSetCfgPrt(uint8_t port, uint8_t outProtocolMask)
{
    if (verbose > 3) {
        emit toConsole(QString("Ublox UBXSetCfgPort running in thread " + QString("0x%1\n").arg((int)syscall(SYS_gettid))));
    }
    unsigned char data[20];
    // initialise all contents from data as 0 first!
    for (int i = 0; i < 20; i++) {
        data[i] = 0;
    }
    if (port >= s_nr_targets) {
        emit UBXCfgError("port > " + QString::number(s_nr_targets - 1) + " is not possible");
        return;
    }
    if (port == 1) {
        // port 1 is UART port, payload for other ports may differ
        // setup payload. Bit masks are set up byte wise from lowest to highest byte
        data[0] = port; // set port (1 Byte)
        data[1] = 0; // reserved
        data[2] = 0;
        data[3] = 0; // txReady options (not used)
        // mode option:
        data[4] = 0b11000000; // charLen option (first 2 bits): 11 means 8 bit character length.
            // (10 means 7 bit character length only with parity enabled)
        data[5] = 0b00001000; // first 2 bits unimportant. 00 -> 1 stop bit. 100 -> no parity. last bit unimportant.
        data[6] = 0;
        data[7] = 0; //part of mode option but no meaning

        // baudrate: (check if it works)
        data[8] = (uint8_t)(((uint32_t)_baudRate) & 0x000000ff);
        data[9] = (uint8_t)((((uint32_t)_baudRate) & 0x0000ff00) >> 8);
        data[10] = (uint8_t)((((uint32_t)_baudRate) & 0x00ff0000) >> 16);
        data[11] = 0; //not needed because baudRate will never be over 16777216 (2^24)

        // inProtoMask enables/disables possible protocols for sending messages to the gps module:
        data[12] = 0b00100111; // () () (RTCM3) () () (RTCM) (NMEA) (UBX)
        data[13] = 0; //has no meaning but part of inProtoMask

        // outProtoMask enables/disables protocols for receiving messages from the gps module:
        data[14] = outProtocolMask;

        data[15] = 0; //has no meaning but part of outProtoMask

        data[16] = 0;
        data[17] = 0; // extendedTxTimeout not needed
        data[18] = 0;
        data[19] = 0; // reserved
    }
    enqueueMsg(UBX_MSG::CFG_PRT, toStdString(data, 20));
}

void QtSerialUblox::UBXSetCfgMsgRate(uint16_t msgID, uint8_t port, uint8_t rate)
{ // set message rate on port. (rate 1 means every intervall the messages is sent)
    // if port = -1 set all ports
    if (verbose > 3) {
        emit toConsole(QString("Ublox UBXsetCfgMsg running in thread " + QString("0x%1\n").arg((int)syscall(SYS_gettid))));
    }
    unsigned char data[8];
    // initialise all contents from data as 0 first!
    for (int i = 0; i < 8; i++) {
        data[i] = 0;
    }
    if (port > 6) {
        emit UBXCfgError("port > 5 is not possible");
    }
    data[0] = (uint8_t)((msgID & 0xff00) >> 8);
    data[1] = msgID & 0xff;
    if (port != 6) {
        data[2 + port] = rate;
    } else {
        for (int i = 2; i < 6; i++) {
            data[i] = rate;
        }
    }

    enqueueMsg(UBX_MSG::CFG_MSG, toStdString(data, 8));
}

void QtSerialUblox::UBXReset(uint32_t resetFlags)
{
    uint16_t navBbrMask = (resetFlags & 0xffff0000) >> 16;
    uint8_t resetMode = resetFlags & 0xff;
    unsigned char data[4];
    data[0] = navBbrMask & 0xff;
    data[1] = (navBbrMask & 0xff00) >> 8;
    data[2] = resetMode;
    data[3] = 0;

    UbxMessage newMessage { UBX_MSG::CFG_RST, toStdString(data, 4) };
    sendUBX(newMessage);
}

void QtSerialUblox::UBXSetMinMaxSVs(uint8_t minSVs, uint8_t maxSVs)
{
    unsigned char data[40];
    data[2] = 0x04; // MinMax flag in mask 1
    data[3] = 0x00; // all other flags are zero
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    data[10] = minSVs;
    data[11] = maxSVs;

    enqueueMsg(UBX_MSG::CFG_NAVX5, toStdString(data, 40));
}

void QtSerialUblox::UBXSetMinCNO(uint8_t minCNO)
{
    unsigned char data[40];
    data[2] = 0x08; // minCNO flag in mask 1
    data[3] = 0x00; // all other flags are zero
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;
    data[12] = minCNO;

    enqueueMsg(UBX_MSG::CFG_NAVX5, toStdString(data, 40));
}

void QtSerialUblox::UBXSetAopCfg(bool enable, uint16_t maxOrbErr)
{
    unsigned char data[40];
    data[2] = 0x00; // aop flag in mask 2
    data[3] = 0x40; // all other flags are zero
    data[4] = 0;
    data[5] = 0;
    data[6] = 0;
    data[7] = 0;

    data[27] = (uint8_t)enable;
    data[30] = maxOrbErr & 0xff;
    data[31] = (maxOrbErr >> 8) & 0xff;

    enqueueMsg(UBX_MSG::CFG_NAVX5, toStdString(data, 40));
}

void QtSerialUblox::UBXSaveCfg(uint8_t devMask)
{
    unsigned char data[13];
    // select the following sections to save:
    // ioPort, msgCfg, navCfg, rxmCfg, antConf
    uint32_t sectionMask = 0x01 | 0x02 | 0x08 | 0x10 | 0x400;

    data[0] = data[1] = data[2] = data[3] = 0x00; // clear mask is all zero
    data[8] = data[9] = data[10] = data[11] = 0x00; // load mask is all zero
    data[4] = sectionMask & 0xff;
    data[5] = (sectionMask >> 8) & 0xff;
    data[6] = (sectionMask >> 16) & 0xff;
    data[7] = (sectionMask >> 24) & 0xff;
    data[12] = devMask;

    enqueueMsg(UBX_MSG::CFG_CFG, toStdString(data, 13));
}

void QtSerialUblox::onRequestGpsProperties()
{
}

void QtSerialUblox::delay(int millisecondsWait)
{
    QEventLoop loop;
    QTimer t;
    t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(millisecondsWait);
    loop.exec();
}

void QtSerialUblox::pollMsgRate(uint16_t msgID)
{
    UbxMessage msg;
    unsigned char temp[2];
    temp[0] = (uint8_t)((msgID & 0xff00) >> 8);
    temp[1] = (uint8_t)(msgID & 0xff);
    enqueueMsg(UBX_MSG::CFG_MSG, toStdString(temp, 2));
}

void QtSerialUblox::pollMsg(uint16_t msgID)
{
    UbxMessage msg;
    unsigned char temp[1];
    switch (msgID) {
    case UBX_MSG::CFG_PRT: // CFG-PRT
        // in this special case "rate" is the port ID
        temp[0] = 1;
        outMsgBuffer.push(UbxMessage { msgID, toStdString(temp, 1) });
        if (!msgWaitingForAck) {
            sendQueuedMsg();
        }
        break;
    case UBX_MSG::MON_VER:
        // the VER message apparently does not confirm reception with an ACK
        sendUBX(UbxMessage { msgID, "" });
        break;
    default:
        // for most messages the poll msg is just the message without payload
        sendUBX(UbxMessage { msgID, "" });
        break;
    }
}

auto QtSerialUblox::getProtVersion() -> double
{
    double verValue = 0.;
    try {
        verValue = std::stod(fProtVersionString);
    } catch (std::exception&) {
    }
    return verValue;
}

void QtSerialUblox::enqueueMsg(uint16_t msgID, const std::string& payload)
{
    UbxMessage newMessage { msgID, payload };
    outMsgBuffer.push(newMessage);
    if (!msgWaitingForAck) {
        sendQueuedMsg();
    }
}
