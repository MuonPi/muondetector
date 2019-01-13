#include <qtserialublox.h>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <QEventLoop>
#include <ublox_messages.h>
using namespace std;


static std::string toStdString(unsigned char* data, int dataSize) {
	std::stringstream tempStream;
	for (int i = 0; i < dataSize; i++) {
		tempStream << data[i];
	}
	return tempStream.str();
}

QtSerialUblox::QtSerialUblox(const QString serialPortName, int newTimeout, int baudRate,
	bool newDumpRaw, int newVerbose, bool newShowout, bool newShowin, QObject *parent) : QObject(parent)
{
	_portName = serialPortName;
	_baudRate = baudRate;
	verbose = newVerbose;
	dumpRaw = newDumpRaw;
	showout = newShowout;
	showin = newShowin;
	timeout = newTimeout;
}

void QtSerialUblox::makeConnection() {
	// this function gets called with a signal from client-thread
	// (QtSerialUblox runs in a separate thread only communicating with main thread through messages)
	if (verbose > 4) {
		emit toConsole(QString("gps running in thread " + QString("0x%1\n").arg((int)this->thread())));
	}
	if (serialPort) {
		delete(serialPort);
	}
	serialPort = new QSerialPort(_portName);
	serialPort->setBaudRate(_baudRate);
	if (!serialPort->open(QIODevice::ReadWrite)) {
		emit gpsConnectionError();
		emit toConsole(QObject::tr("Failed to open port %1, error: %2\ntrying again in %3 seconds\n")
			.arg(_portName)
			.arg(serialPort->errorString())
			.arg(timeout));
		delete serialPort;
		delay(timeout);
		emit gpsRestart();
		this->deleteLater();
		return;
	}
	ackTimer = new QTimer();
	connect(ackTimer, &QTimer::timeout, this, &QtSerialUblox::ackTimeout);
	connect(serialPort, &QSerialPort::readyRead, this, &QtSerialUblox::onReadyRead);
	serialPort->clear(QSerialPort::AllDirections);
	if (verbose == 1) {
		emit toConsole("rising               falling               accEst valid timebase utc\n");
	}
}

void QtSerialUblox::sendQueuedMsg(bool afterTimeout) {
	if (afterTimeout) {
		if (!msgWaitingForAck) {
			return;
		}
		ackTimer->start(timeout);
		sendUBX(*msgWaitingForAck);
		return;
	}
	if (outMsgBuffer.empty()) { return; }
	if (msgWaitingForAck) {
		if (verbose > 0) {
			emit toConsole("tried to send queued message but ack for previous message not yet received\n");
		}
		return;
	}
	msgWaitingForAck = new UbxMessage;
	*msgWaitingForAck = outMsgBuffer.front();
	ackTimer->setSingleShot(true);
	ackTimer->start(timeout);
	sendUBX(*msgWaitingForAck);
	outMsgBuffer.pop();
}

void QtSerialUblox::ackTimeout() {
	if (!msgWaitingForAck) {
		return;
	}
	if (verbose > 1) {
		std::stringstream tempStream;
		tempStream << "ack timeout, trying to resent message 0x" << std::setfill('0') << std::setw(2) << hex
			<< ((msgWaitingForAck->msgID & 0xff00) >> 8) << " 0x" << std::setfill('0') << std::setw(2) << hex << (msgWaitingForAck->msgID & 0x00ff);
		for (unsigned int i = 0; i < msgWaitingForAck->data.length(); i++) {
			tempStream << " 0x" << std::setfill('0') << std::setw(2) << hex << (int)(msgWaitingForAck->data[i]);
		}
		tempStream << endl;
		emit toConsole(QString::fromStdString(tempStream.str()));
	}
	sendQueuedMsg(true);
}

void QtSerialUblox::onReadyRead() {
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
	if (scanUnknownMessage(buffer, message)) {
		// so it found a message therefore we can now process the message
		if (showin) {
			std::stringstream tempStream;
			tempStream << " in: ";
			int classID = (int)((message.msgID & 0xff00) >> 8);
			int msgID = (int)(message.msgID & 0xff);
			tempStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << classID;
			tempStream << " 0x" << std::setfill('0') << std::setw(2) << std::hex << msgID << " ";
			for (std::string::size_type i = 0; i < message.data.length(); i++) {
				tempStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << (int)(message.data[i]) << " ";
			}
			tempStream << "\n";
			emit toConsole(QString::fromStdString(tempStream.str()));
		}
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
	else {
		// discard everything before the start of a NMEA message, too
		// to ensure that buffer won't grow too big
		if (discardAllNMEA) {
			std::string beginNMEA = "$";
			size_t found = 0;
			while (found != string::npos) {
				found = buffer.find(beginNMEA);
				if (found == string::npos) {
					break;
				}
				buffer.erase(0, found + 1);
			}
		}
		return false;
	}
	// message.classID = (uint8_t)mess[2];
	// message.messageID = (uint8_t)mess[3];
	message.msgID = (uint16_t)mess[2] << 8;
	message.msgID += (uint16_t)mess[3];
	if (mess.size() < 8) return false;
	int len = (int)(mess[4]);
	len += ((int)(mess[5])) << 8;
	if (((long int)mess.size() - 8) < len) {
		std::string::size_type found = buffer.find(refstr, 2);
		if (found != string::npos) {
			if (verbose > 1) {
				std::stringstream tempStream;
				tempStream << "received faulty UBX string:\n " << dec;
				for (std::string::size_type i = 0; i < found; i++) tempStream
					<< setw(2) << hex << "0x" << (int)buffer[i] << " ";
				tempStream << endl;
				emit toConsole(QString::fromStdString(tempStream.str()));
			}
			buffer.erase(0, found);
		}
		return false;
	}
	buffer.erase(0, len + 8);

	unsigned char chkA;
	unsigned char chkB;
	calcChkSum(mess.substr(0, len + 6), &chkA, &chkB);
	if (mess[len + 6] == chkA && mess[len + 7] == chkB)
	{
		message.data = mess.substr(6, len);
		return true;
	}

	return false;
}

bool QtSerialUblox::sendUBX(uint16_t msgID, std::string& payload, uint16_t nBytes)
{
	//    std::cout << "0x"<< std::setfill('0') << std::setw(2) << std::hex << ((msgID & 0xff00) >> 8)
	//                     << std::setfill('0') << std::setw(2) << std::hex << (msgID & 0xff);
	//    for (int i = 0; i<nBytes; i++){
	//        std::cout << " 0x" << std::setfill('0') << std::setw(2) << std::hex << (int)payload[i];
	//    }
	//    std::cout<<std::endl;
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
	if (serialPort) {
		QByteArray block(s.c_str(), s.size());
		if (serialPort->write(block)) {
			if (serialPort->waitForBytesWritten(timeout)) {
				if (showout) {
					std::stringstream tempStream;
					tempStream << "out: ";
					for (std::string::size_type i = 2; i < s.length(); i++) {
						tempStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << (int)s[i] << " ";
					}
					tempStream << "\n";
					emit toConsole(QString::fromStdString(tempStream.str()));
				}
				return true;
			}
			else {
				emit toConsole("wait for bytes written timeout while trying to write to serialPort");
			}
		}
		else {
			emit toConsole("error writing to serialPort");
		}
	}
	else {
		emit toConsole("error: serialPort not instantiated");
	}
	return false;
}
bool QtSerialUblox::sendUBX(uint16_t msgID, unsigned char* payload, uint16_t nBytes) {
	std::stringstream payloadStream;
	std::string payloadString;
	for (int i = 0; i < nBytes; i++) {
		payloadStream << payload[i];
	}
	payloadString = payloadStream.str();
	return sendUBX(msgID, payloadString, nBytes);
}

bool QtSerialUblox::sendUBX(UbxMessage &msg) {
	return sendUBX(msg.msgID, msg.data, msg.data.size());
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

void QtSerialUblox::UBXSetCfgRate(uint16_t measRate, uint16_t navRate)
{
	if (verbose > 4) {
		emit toConsole(QString("Ublox UBXsetCfgRate running in thread " + QString("0x%1\n")
			.arg((int)this->thread())));
	}
	unsigned char data[6];
	// initialise all contents from data as 0 first!
	for (int i = 0; i < 6; i++) {
		data[i] = 0;
	}
	if (measRate < 10 || navRate < 1 || navRate>127) {
		emit UBXCfgError("measRate <10 || navRate<1 || navRate>127 error");
	}
	data[0] = measRate & 0xff;
	data[1] = (measRate & 0xff00) >> 8;
	data[2] = navRate & 0xff;
	data[3] = (navRate & 0xff00) >> 8;
	data[4] = 0;
	data[5] = 0;

	UbxMessage newMessage;
	newMessage.msgID = MSG_CFG_RATE;
	newMessage.data = toStdString(data, 6);
	outMsgBuffer.push(newMessage);
	if (!msgWaitingForAck) {
		sendQueuedMsg();
	}
	// sendUBX(MSG_CFG_RATE, data, sizeof(data));
	/* Ack check has to be done differently, asynchronously
	if (waitAck(MSG_CFG_RATE, 10000))
	{
		emit toConsole("Set CFG successful");
	}
	else {
		emit UBXCfgError("Set CFG timeout");
	}*/
}

void QtSerialUblox::UBXSetCfgPrt(uint8_t port, uint8_t outProtocolMask) {
	if (verbose > 4) {
		emit toConsole(QString("Ublox UBXSetCfgPort running in thread " + QString("0x%1\n")
			.arg((int)this->thread())));
	}
	unsigned char data[20];
	// initialise all contents from data as 0 first!
	for (int i = 0; i < 20; i++) {
		data[i] = 0;
	}
	if (port > 6) {
		emit UBXCfgError("port > 5 is not possible");
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
		data[8] = (uint8_t)(((uint16_t)_baudRate) & 0x00ff);
		data[9] = (uint8_t)((((uint16_t)_baudRate) & 0xff00) >> 8);
		data[10] = 0;
		data[11] = 0; //not needed because baudRate will never be over 65536 (2^16)

		// inProtoMask enables/disables possible protocols for sending messages to the gps module:
		data[12] = 0b00100111; // () () (RTCM3) () () (RTCM) (NMEA) (UBX)
		data[13] = 0; //has no meaning but part of inProtoMask

		// outProtoMask enables/disables protocols for receiving messages from the gps module:
		//data[14] = 0b00100011; // () () (RTCM3) () () () (NMEA) (UBX) FOR EXAMPLE
		data[14] = outProtocolMask;

		data[15] = 0; //has no meaning but part of outProtoMask

		data[16] = 0;
		data[17] = 0; // extendedTxTimeout not needed
		data[18] = 0;
		data[19] = 0; // reserved
	}
	// sendUBX(MSG_CFG_PRT, data, 20);
	UbxMessage newMessage;
	newMessage.msgID = MSG_CFG_PRT;
	newMessage.data = toStdString(data, 20);
	outMsgBuffer.push(newMessage);
	if (!msgWaitingForAck) {
		sendQueuedMsg();
	}
}

void QtSerialUblox::UBXSetCfgMsgRate(uint16_t msgID, uint8_t port, uint8_t rate)
{ // set message rate on port. (rate 1 means every intervall the messages is sent)
	// if port = -1 set all ports
	if (verbose > 4) {
		emit toConsole(QString("Ublox UBXsetCfgMsg running in thread " + QString("0x%1\n")
			.arg((int)this->thread())));
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
	//   cout<<"data[0]="<<(int)data[0]<<" data[1]="<<(int)data[1]<<endl;
	if (port != 6) {
		data[2 + port] = rate;
	}
	else {
		for (int i = 2; i < 6; i++) {
			data[i] = rate;
		}
	}
	//sendUBX(MSG_CFG_MSG, data, 8);#
	UbxMessage newMessage;
	newMessage.msgID = MSG_CFG_MSG;
	newMessage.data = toStdString(data, 8);
	outMsgBuffer.push(newMessage);
	if (!msgWaitingForAck) {
		sendQueuedMsg();
	}
	/* Ack check has to be done differently, asynchronously
	if (waitAck(MSG_CFG_MSG, 12000)) {
		emit toConsole("Set CFG successful");
	}
	else {
		emit UBXCfgError("Set CFG timeout");
	}
	*/
}

void QtSerialUblox::onRequestGpsProperties(){

}

void QtSerialUblox::delay(int millisecondsWait)
{
	QEventLoop loop;
	QTimer t;
	t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
	t.start(millisecondsWait);
	loop.exec();
}

void QtSerialUblox::handleError(QSerialPort::SerialPortError serialPortError)
{
	if (serialPortError == QSerialPort::ReadError) {
		emit toConsole(QObject::tr("An I/O error occurred while reading "
			"the data from port %1, error: %2\n")
			.arg(serialPort->portName())
			.arg(serialPort->errorString()));
	}
}
void QtSerialUblox::pollMsgRate(uint16_t msgID) {
	UbxMessage msg;
	unsigned char temp[2];
	temp[0] = (uint8_t)((msgID & 0xff00) >> 8);
	temp[1] = (uint8_t)(msgID & 0xff);
	msg.msgID = MSG_CFG_MSG;
	msg.data = toStdString(temp, 2);
	outMsgBuffer.push(msg);
	if (!msgWaitingForAck) {
		sendQueuedMsg();
	}
}

void QtSerialUblox::pollMsg(uint16_t msgID) {
	UbxMessage msg;
	unsigned char temp[1];
	switch (msgID) {
	case MSG_CFG_PRT: // CFG-PRT
		// in this special case "rate" is the port ID
		temp[0] = 1;
		msg.msgID = msgID;
		msg.data = toStdString(temp, 1);
		outMsgBuffer.push(msg);
		if (!msgWaitingForAck) {
			sendQueuedMsg();
		}
		break;
	default:
		// for most messages the poll msg is just the message without payload
		break;
	}
}
