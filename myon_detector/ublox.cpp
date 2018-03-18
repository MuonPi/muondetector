#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <iterator>     // std::distance
#include <chrono>
//#include <SerialStream.h>
//#include <SerialPort.h>
#include <QEventLoop>
#include <QTimer>
#include <QTextStream>
#include "gnsssatellite.h"
#include "ublox.h"
#include "unixtime_from_gps.h"


//using namespace LibSerial;
using namespace std;

const int	TIMEOUT = 100;
const bool 	BLOCKING = true;
const int	MSGTIMEOUT = 1500;


/* Inverse function to get back from RTC 'DMY HMS' form to time_t UTC
   form.  This essentially uses mktime(), but involves some awful
   complexity to cope with timezones.  The problem is that mktime's
   behaviour with regard to the daylight saving flag in the 'struct
   tm' does not seem to be reliable across all systems, unless that
   flag is set to zero.

   tm_isdst = -1 does not seem to work with all libc's - it is treated
   as meaning there is DST, or fails completely.  (It is supposed to
   use the timezone info to work out whether summer time is active at
   the specified epoch).

   tm_isdst = 1 fails if the local timezone has no summer time defined.

   The approach taken is as follows.  Suppose the RTC is on localtime.
   We perform all mktime calls with the tm_isdst field set to zero.

   Let y be the RTC reading in 'DMY HMS' form.  Let M be the mktime
   function with tm_isdst=0 and L be the localtime function.

   We seek x such that y = L(x).  Now there will exist a value Z(t)
   such that M(L(t)) = t + Z(t) for all t, where Z(t) depends on
   whether daylight saving is active at time t.

   We want L(x) = y.  Therefore M(L(x)) = x + Z = M(y).  But
   M(L(M(y))) = M(y) + Z.  Therefore x = M(y) - Z = M(y) - (M(L(M(y)))
   - M(y)).

   The case for the RTC running on UTC is identical but without the
   potential complication that Z depends on t.
*/



//972(2)26714457
//6544262211
//0544262211
//0097226714457
//02


Ublox::Ublox(const std::string& serialPortName, int baudRate,
             QObject *parent, bool dumpRaw, int verbose)
             : QObject(parent)// : QThread(parent)
{
	_portName = serialPortName;
	_baudRate = baudRate;
    fVerbose = verbose;
    fDumpRaw = dumpRaw;
}

Ublox::~Ublox()
{
	Disconnect();
    if (!_port){
        delete _port;
    }
}

void Ublox::loop()//run()
{
    std::string buffer = "";
    mutex.lock();
    bool q = quit;
    int verbose = fVerbose;
    bool dumpraw = fDumpRaw;
    mutex.unlock();
    if (verbose>2){
        std::stringstream tempStream;
        tempStream << "Ublox loop reporting from thread " << this->thread();
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
    while (!q) {
		std::string answer = "";
        std::string buf = "";
        int n = 0;
        n = ReadBuffer(buf);
        mutex.lock();
        q = quit;
        mutex.unlock();
        while (!buf.size() && !q) {
            if (verbose > 3){
                 emit toConsole("no data...sleep for 100ms");
            }
            //std::this_thread::yield();
            //std::this_thread::sleep_for(std::chrono::milliseconds(100));
            delay(100);
            n = ReadBuffer(buf);
            if (dumpraw){
                //cout << buf;
                emit toConsole(QString::fromStdString(buf));
            }
        }
        buffer += buf;
		UbxMessage message;
        answer = scanUnknownMessage(buffer, message.msgID);
        mutex.lock();
        q = quit;
        mutex.unlock();
        if (answer.size() && !q) {
            message.data = answer;
            processMessage(message, verbose);      
            if (verbose > 2) {
                std::stringstream tempStream;
				// 	cout<<"received UBX message "<<hex<<(int)message.classID<<" "<<hex<<(int)message.messageID<<" : ";
                tempStream << "received UBX message " << "0x" << hex << setw(4) << setfill('0') << message.msgID << " : ";
                for (std::string::size_type i = 0; i < answer.size(); i++) tempStream << hex << setw(2) << setfill('0') << (int)answer[i] << " ";
                string temp;
                //tempStream >> temp;
                emit toConsole(QString::fromStdString(tempStream.str()));
			}
            answer = scanUnknownMessage(buffer, message.msgID);
            //mutex.lock();
            /*
            if (fMessageBuffer.size() > MAX_MESSAGE_BUFSIZE){
                fMessageBuffer.pop_front();
                emit toConsole("message buffer overload, data loss!!");
            }
            if (verbose > 3){
                emit toConsole("message buffer size: "+fMessageBuffer.size());
            }*/
			//     }

            //mutex.unlock();
		}
		//     while (fMessageBuffer.size()>MAX_MESSAGE_BUFSIZE) fMessageBuffer.erase(fMessageBuffer.begin());
        //std::this_thread::yield();
        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
        delay(10);
    }
    if (verbose > 2) {
        emit toConsole("finishing read thread");
    }
}

void Ublox::Connect()
{
    mutex.lock();
    if (_port){
        if (fVerbose > 3){
            emit toConsole(QString::fromStdString("port already exists, delete old port"));
        }
        Disconnect();
        delete _port;
    }
    mutex.unlock();
    _port = new Serial(_portName, _baudRate, 8, 1, 4096, Serial::NONE, BLOCKING);
    if (fVerbose > 2){
        std::stringstream tempStream;
        tempStream << "Ublox Connect reporting from thread " << this->thread();
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
	if (!_port->isOpen())
		_port->open();
	_port->setTimeout(TIMEOUT / 100);
	//      _port->setTimeout(0);
	/*
	   _port->Open(SerialPort::BAUD_57600,
				   SerialPort::CHAR_SIZE_8,
				   SerialPort::PARITY_NONE,
				   SerialPort::STOP_BITS_1,
				   SerialPort::FLOW_CONTROL_NONE);
	*/

	if (!_port->isOpen())
	{
        emit toConsole(QString::fromStdString("Error opening Port"+_portName));
    }

	_port->flushPort();
	quit = false;
    //mutex.unlock();
    loop();
}

void Ublox::Disconnect()
{
    mutex.lock();
    quit = true;
	while (_port->isOpen()) {
		_port->closePort();
	}
    mutex.unlock();
}

void Ublox::delay(int millisecondsWait)
{
	QEventLoop loop;
	QTimer t;
	t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
	t.start(millisecondsWait);
	loop.exec();
}

void Ublox::processMessage(const UbxMessage& msg, int verbose)
{
	uint8_t classID = (msg.msgID & 0xff00) >> 8;
	uint8_t messageID = msg.msgID & 0xff;

	std::vector<GnssSatellite> sats;
    std::stringstream tempStream;
    //std::string temp;

	switch (classID) {
	case 0x01: // UBX-NAV
		switch (messageID) {
		case 0x20:
            if (verbose) {
                tempStream << "received UBX-NAV-TIMEGPS message (" << hex
                           << (int)classID << " " << hex << (int)messageID << ")";
			}
            UBXNavTimeGPS(msg.data, verbose);
			break;
		case 0x21:
            if (verbose) {
                tempStream << "received UBX-NAV-TIMEUTC message (" << hex
                           << (int)classID << " " << hex << (int)messageID << ")";
            }
            UBXNavTimeUTC(msg.data, verbose);
			break;
		case 0x22:
            if (verbose) {
                tempStream << "received UBX-NAV-CLOCK message (" << hex
                           << (int)classID << " " << hex << (int)messageID << ")" << endl;
			}
            UBXNavClock(msg.data, verbose);
			break;
		case 0x35:
            sats = UBXNavSat(msg.data, true, verbose);
            if (verbose) {
                tempStream << "received UBX-NAV-SAT message (" << hex << (int)classID
                           << " " << hex << (int)messageID << ")" << endl;
			}
            //mutex.lock();
            emit gpsPropertyUpdatedGnss(sats, satList.updateAge());
            satList = sats;
            //mutex.unlock();
			//break;
//				tempStream<<"Satellite List: "<<sats.size()<<" sats received"<<endl;
// 				GnssSatellite::PrintHeader(true);
// 				for (int i=0; i<sats.size(); i++) sats[i].Print(i, false);
			break;
		default:
            if (verbose) {
                tempStream << "received unhandled UBX-NAV message (" << hex << (int)classID
                           << " " << hex << (int)messageID << ")" << endl;
			}
		}
		break;
	case 0x02: // UBX-RXM
		break;
	case 0x05: // UBX-ACK
		break;
	case 0x0b: // UBX-AID
		break;
	case 0x06: // UBX-CFG
		switch (messageID) {
		default:
            if (verbose) {
                tempStream << "received unhandled UBX-CFG message (" << hex << (int)classID
                           << " " << hex << (int)messageID << ")" << endl;
			}
		}
		break;
	case 0x10: // UBX-ESF
		break;
	case 0x28: // UBX-HNR
		break;
	case 0x04: // UBX-INF
		switch (messageID) {
		default:
            if (verbose) {
                tempStream << "received unhandled UBX-INF message (" << hex << (int)classID
                           << " " << hex << (int)messageID << ")" << endl;
			}
		}
		break;
	case 0x21: // UBX-LOG
		break;
	case 0x13: // UBX-MGA
		break;
	case 0x0a: // UBX-MON
		switch (messageID) {
		case 0x08:
            if (verbose) {
                tempStream << "received UBX-MON-TXBUF message (" << hex << (int)classID
                           << " " << hex << (int)messageID << ")" << endl;
			}
            UBXMonTx(msg.data, verbose);
			break;
		case 0x09:
            if (verbose) {
                tempStream << "received UBX-MON-HW message (" << hex << (int)classID
                           << " " << hex << (int)messageID << ")" << endl;
			}
            UBXMonHW(msg.data, verbose);
			break;
		default:
            if (verbose) {
                tempStream << "received unhandled UBX-MON message (" << hex << (int)classID
                           << " " << hex << (int)messageID << ")" << endl;
			}
		}
		break;
	case 0x27: // UBX-SEC
		break;
	case 0x0d: // UBX-TIM
		switch (messageID) {
		case 0x01: // UBX-TIM-TP
            if (verbose) {
                tempStream << "received UBX-TIM-TP message (" << hex << (int)classID << " "
                           << hex << (int)messageID << ")" << endl;
			}
            UBXTimTP(msg.data, verbose);
			break;
		case 0x03: // UBX-TIM-TM2
            if (verbose) {
                tempStream << "received UBX-TIM-TM2 message (" << hex << (int)classID << " "
                           << hex << (int)messageID << ")" << endl;
			}
            UBXTimTM2(msg.data, verbose);
			break;
		default:
            if (verbose) {
                tempStream << "received unhandled UBX-TIM message (" << hex << (int)classID
                           << " " << hex << (int)messageID << ")" << endl;
			}
		}
		break;
	case 0x09: // UBX-UPD
		break;
	default: break;
	}
    //tempStream >> temp;
    emit toConsole(QString::fromStdString(tempStream.str()));
}

unsigned int Ublox::ReadBuffer(std::string& buf)
{
	//   std::string s="";
	//   if (!_port->IsDataAvailable()) return s;
	int n = 0;
	//try
    //{
		// Wait for Data and read
		std::string s1;
        //mutex.lock();
		n = _port->read(buf, 250);
        //mutex.unlock();
		//      cout<<s1<<endl;
    //}
	/*
	   catch (...)
	   {
		  cerr<<"Timeout!"<<endl;
	   }*/

	return n;
}

unsigned int Ublox::WriteBuffer(const std::string& buf)
{
	//   std::string s="";
	//   if (!_port->IsDataAvailable()) return s;
	int n = 0;
    //try
    //{
    //mutex.lock();
    n = _port->write(buf);
    //mutex.unlock();
    //}
		//      cout<<s1<<endl;
	//usleep(100000L);
 /*
	catch (...)
	{
	   cerr<<"Timeout!"<<endl;
	}*/

	return n;
}

void Ublox::calcChkSum(const std::string& buf, unsigned char* chkA, unsigned char* chkB)
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

bool Ublox::sendUBX(uint16_t msgID, unsigned char* payload, int nBytes)
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
	if (s.size() == WriteBuffer(s)) return true;
	return false;
}

bool Ublox::sendUBX(unsigned char classID, unsigned char messageID, unsigned char* payload, int nBytes)
{
    uint16_t msgID = (messageID + (uint16_t)classID) << 8;
	return sendUBX(msgID, payload, nBytes);
}


bool Ublox::waitAck(int timeout, int verbose)
{
	int iter = 0;
	std::string answer;
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
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        delay(1);
        // 		usleep(1000);
		iter++;
        //mutex.lock();
		for (std::list<UbxMessage>::iterator it = fMessageBuffer.begin(); it != fMessageBuffer.end(); it++) {
			// 			if (it->classID==0x05 && it->messageID<2) {
			if (it->msgID >= 0x0500 && it->msgID < 0x0502) {
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
        tempStream << "received UBX ACK string: ";
        for (std::string::size_type i = 0; i < answer.size(); i++) tempStream << hex << (int)answer[i] << " ";
        //std::string temp;
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}

	return true;
}

bool Ublox::waitAck(uint16_t msgID, int timeout, int verbose)
{
	int iter = 0;
	const static int cycleSleepMs = 10;
	std::string answer;
	int mid = -1;
	while (iter < timeout / cycleSleepMs && !answer.size())
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
        //std::this_thread::sleep_for(std::chrono::milliseconds(cycleSleepMs));
        delay(cycleSleepMs);
        // 		usleep(1000);
		iter++;
        //mutex.lock();
		for (std::list<UbxMessage>::iterator it = fMessageBuffer.begin(); it != fMessageBuffer.end(); it++) {
			if (it->msgID >= 0x0500 && it->msgID < 0x0502) {
                if (verbose > 4) {
                    std::stringstream tempStream;
                    //std::string temp;
                    tempStream << "MSG-ACK ok: data[0]=" << (int)it->data[0] << " data[1]=" << (int)it->data[1] << " size=" << it->data.size() << " " << hex << msgID;
                    //tempStream >> temp;
                    emit toConsole(QString::fromStdString(tempStream.str()));
                }
				if (it->data.size() == 2 && it->data[0] == (uint8_t)((msgID & 0xff00) >> 8) && it->data[1] == (msgID & 0xff)) {
					mid = it->msgID;
					answer = it->data;
					fMessageBuffer.erase(it);
					break;
				}
			}
		}
        //mutex.unlock();
	}
	if (iter >= timeout / cycleSleepMs) return false;

    if (verbose > 2) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "received UBX " << std::string((mid == 0x0501) ? "ACK" : "NAK") << " message";
        if (verbose > 3) {
            tempStream << ": ";
            for (std::string::size_type i = 0; i < answer.size(); i++) tempStream << hex << (int)answer[i] << " ";
		}
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
	return true;
}

bool Ublox::waitAck(uint8_t classID, uint8_t messageID, int timeout, int verbose)
{
    uint16_t msgID = (messageID + (uint16_t)classID) << 8;
    return waitAck(msgID, timeout, verbose);
}

bool Ublox::pollUBX(uint16_t msgID, std::string& answer, int timeout, int verbose)
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

		// 		usleep(1000);
        //std::this_thread::sleep_for(std::chrono::milliseconds(1));
        this->thread()->sleep(1);
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

bool Ublox::pollUBX(uint8_t classID, uint8_t messageID, std::string& answer, int timeout, int verbose)
{
    uint16_t msgID = (messageID + (uint16_t)classID) << 8;
    return pollUBX(msgID, answer, timeout, verbose);
}

// bool Ublox::pollUBX(uint16_t msgID, uint8_t specifier, std::string& answer, int timeout)
// {
//     sendUBX(msgID, &specifier, 1);
// 
//     int iter=0;
//     answer="";
//     std::string buffer="";
//     while (iter<timeout && !answer.size())
//     {
// /*
// 		std::string buf="";
// 		int n=ReadBuffer(buf);
// 		if (n>0) {
// 			buffer+=buf;
// 			iter++;
// 			answer=scanMessage(buffer,classID,messageID);
// 		}
// */
//     
// 		std::this_thread::sleep_for(std::chrono::milliseconds(1));    
// // 		usleep(1000);
// 		iter++;
// 		for (std::list<UbxMessage>::iterator it=fMessageBuffer.begin(); it!=fMessageBuffer.end(); it++) {
// 			if (it->msgID==msgID) {
// 				answer=it->data;
// 				fMessageBuffer.erase(it);
// 				break;
// 			} 
// 		}
//     }
// 
//     if (iter>=timeout) return false;
// 
// /*
//     int iter=0;
//     answer="";
//     std::string buffer="";
//     while (iter<timeout && !answer.size())
//     {
// 	std::string buf="";
//         int n=ReadBuffer(buf);
// 	if (n>0) {
// 	  buffer+=buf;
// 	  iter++;
// 	  answer=scanMessage(buffer,classID,messageID);
// 	}
//     }
// */
//     if (verbose>2) {
//       cout<<"received UBX string: ";
//       for (int i=0; i<answer.size(); i++) cout<<hex<<(int)answer[i]<<" ";
//       cout<<endl;
//     }
//     if (answer.size())  return true;
//     return false;
// }
// 
// bool Ublox::pollUBX(uint8_t classID, uint8_t messageID, uint8_t specifier, std::string& answer, int timeout)
// {
//   uint16_t msgID = messageID + (uint16_t)classID<<8;
//   return pollUBX(msgID,specifier,answer,timeout);
// }

/*
std::string Ublox::scanMessage(const std::string& buffer, uint8_t classID, uint8_t messageID)
{
	std::string refstr="";
	refstr+=0xb5; refstr+=0x62;
	refstr+=classID; refstr+=messageID;

	int found=buffer.find(refstr);
	std::string mess="";
	bool result=false;
	if (found<buffer.size())
	{
	  mess=buffer.substr(found, buffer.size());
	  //result=true;
	}
	if (mess.size()<8) return string();
//       cout<<"received UBX string: ";
//       for (int i=0; i<mess.size(); i++) cout<<hex<<(int)mess[i]<<" ";
//       cout<<endl;
	int len=(int)(mess[4]);
	len+=((int)(mess[5]))<<8;
//    cout<<"len="<<dec<<len<<endl;
	if (mess.size()-8<len) return string();
	unsigned char chkA;
	unsigned char chkB;
	calcChkSum(mess.substr(0,len+6), &chkA, &chkB);
//     cout<<"chkA/B="<<hex<<(int)chkA<<" "<<(int)chkB<<endl;
//     cout<<"mess[len+6/7]="<<hex<<(int)mess[len+6]<<" "<<(int)mess[len+7]<<endl;
	if (mess[len+6]==chkA && mess[len+7]==chkB)
	  return mess.substr(6,len);

	return string();
}
*/

std::string Ublox::scanUnknownMessage(std::string& buffer, uint16_t& msgID)
{   // gets the (maybe not finished) buffer and checks for messages in it that make sense
	if (buffer.size() < 9) return string();
	std::string refstr = "";
	refstr += 0xb5; refstr += 0x62;
	//    cout<<"hier"<<endl;
    std::string::size_type found = buffer.find(refstr);
	std::string mess = "";
    //bool result = false;
	if (found != string::npos)
	{
		mess = buffer.substr(found, buffer.size());
		//	    cout<<"da1"<<endl;
		buffer.erase(0, found);
		//result=true;
	}
	else return string();
	msgID = (uint16_t)mess[2] << 8;
	msgID += (uint16_t)mess[3];
	if (mess.size() < 8) return string();
	//        cout<<"received UBX string: ";
	//        for (int i=0; i<mess.size(); i++) cout<<hex<<(int)mess[i]<<" ";
	//        cout<<endl;
	int len = (int)(mess[4]);
	len += ((int)(mess[5])) << 8;
    if (((long int)mess.size() - 8) < len) {
        std::string::size_type found = buffer.find(refstr, 2);
		if (found != string::npos) {
            std::stringstream tempStream;
            //std::string temp;
            tempStream << "received faulty UBX string:\n " << dec;
            for (std::string::size_type i = 0; i < found; i++) tempStream << hex << (int)buffer[i] << " ";
            //tempStream >> temp;
            emit toConsole(QString::fromStdString(tempStream.str()));
			buffer.erase(0, found);
		}
		return string();
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
		return mess.substr(6, len);

	}

	return string();
}

std::string Ublox::scanUnknownMessage(std::string& buffer, uint8_t& classID, uint8_t& messageID)
{
    uint16_t msgID = (messageID + (uint16_t)classID) << 8;
	return scanUnknownMessage(buffer, msgID);
}

void Ublox::Print()
{
    std::stringstream tempStream;
    //std::string temp;
    //mutex.lock();
    tempStream << "U-Blox-Device:\n"
               << /*std::cout<<setfill('0')" +*/ " serial port    : "
               << _portName;
    //mutex.unlock();
    //tempStream >> temp;
    emit toConsole (QString::fromStdString(tempStream.str()));
}

bool Ublox::UBXNavClock(uint32_t& itow, int32_t& bias, int32_t& drift,
                        uint32_t& tAccuracy, uint32_t& fAccuracy, int verbose)
{
    // why tAccuracy?
    tAccuracy=0;
	std::string answer;
	// UBX-NAV-CLOCK: clock solution
    bool ok = pollUBX(MSG_NAV_CLOCK, answer, MSGTIMEOUT, verbose);
	if (!ok) return ok;
	//return true;
	// parse all fields
	// GPS time of week
	uint32_t iTOW = (int)answer[0];
	iTOW += ((int)answer[1]) << 8;
	iTOW += ((int)answer[2]) << 16;
	iTOW += ((int)answer[3]) << 24;
	itow = iTOW / 1000;
	// clock bias
    if (verbose > 3) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "clkB[0]=" << hex << (int)answer[4] << endl;
        tempStream << "clkB[1]=" << hex << (int)answer[5] << endl;
        tempStream << "clkB[2]=" << hex << (int)answer[6] << endl;
        tempStream << "clkB[3]=" << hex << (int)answer[7] << endl;
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
	int32_t clkB = (int)answer[4];
	clkB += ((int)answer[5]) << 8;
	clkB += ((int)answer[6]) << 16;
	clkB += ((int)answer[7]) << 24;
	bias = clkB;
	// clock drift
	int32_t clkD = (int)answer[8];
	clkD += ((int)answer[9]) << 8;
	clkD += ((int)answer[10]) << 16;
	clkD += ((int)answer[11]) << 24;
	drift = clkD;
	// time accuracy estimate
	uint32_t tAcc = (int)answer[12];
	tAcc += ((int)answer[13]) << 8;
	tAcc += ((int)answer[14]) << 16;
	tAcc += ((int)answer[15]) << 24;
	//  tAccuracy=tAcc;
	  // freq accuracy estimate
	uint32_t fAcc = (int)answer[16];
	fAcc += ((int)answer[17]) << 8;
	fAcc += ((int)answer[18]) << 16;
	fAcc += ((int)answer[19]) << 24;
	fAccuracy = fAcc;
    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-NAV-CLOCK message:" << endl;
        tempStream << " iTOW          : " << dec << iTOW / 1000 << " s" << endl;
        tempStream << " clock bias    : " << dec << clkB << " ns" << endl;
        tempStream << " clock drift   : " << dec << clkD << " ns/s" << endl;
        tempStream << " time accuracy : " << dec << tAcc << " ns" << endl;
        tempStream << " freq accuracy : " << dec << fAcc << " ps/s" << endl;
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
	return ok;
}

bool Ublox::UBXTimTP(uint32_t& itow, int32_t& quantErr, uint16_t& weekNr, int verbose)
{
	std::string answer;
	// UBX-TIM-TP: time pulse timedata
    bool ok = pollUBX(MSG_TIM_TP, answer, MSGTIMEOUT, verbose);
	if (!ok) return ok;
	// parse all fields
	// TP time of week, ms
	uint32_t towMS = (int)answer[0];
	towMS += ((int)answer[1]) << 8;
	towMS += ((int)answer[2]) << 16;
	towMS += ((int)answer[3]) << 24;
	// TP time of week, sub ms
	uint32_t towSubMS = (int)answer[4];
	towSubMS += ((int)answer[5]) << 8;
	towSubMS += ((int)answer[6]) << 16;
	towSubMS += ((int)answer[7]) << 24;
	itow = towMS / 1000;
	// quantization error
	int32_t qErr = (int)answer[8];
	qErr += ((int)answer[9]) << 8;
	qErr += ((int)answer[10]) << 16;
	qErr += ((int)answer[11]) << 24;
	quantErr = qErr;
	// week number
	uint16_t week = (int)answer[12];
	week += ((int)answer[13]) << 8;
	weekNr = week;
	// flags
	uint8_t flags = answer[14];
	// ref info
	uint8_t refInfo = answer[15];

	double sr = towMS / 1000.;
	sr = sr - towMS / 1000;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-TIM-TP message:" << endl;
        tempStream << " tow s            : " << dec << towMS / 1000. << " s" << endl;
        tempStream << " tow sub s        : " << dec << towSubMS << " = " << (long int)(sr*1e9 + towSubMS + 0.5) << " ns" << endl;
        tempStream << " quantization err : " << dec << qErr << " ps" << endl;
        tempStream << " week nr          : " << dec << week << endl;
        tempStream << " flags            : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << " refInfo          : ";
        for (int i = 7; i >= 0; i--) if (refInfo & 1 << i) tempStream << i; else tempStream << "-";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
	return ok;
}

bool Ublox::UBXTimTP(int verbose)
{
	std::string answer;
	// UBX-TIM-TP: time pulse timedata
    bool ok = pollUBX(MSG_TIM_TP, answer, MSGTIMEOUT, verbose);
	if (!ok) return ok;
    UBXTimTP(answer,verbose);
	return ok;
}

bool Ublox::UBXTimTP(const std::string& msg, int verbose)
{
	// parse all fields
	// TP time of week, ms
	uint32_t towMS = (int)msg[0];
	towMS += ((int)msg[1]) << 8;
	towMS += ((int)msg[2]) << 16;
	towMS += ((int)msg[3]) << 24;
	// TP time of week, sub ms
	uint32_t towSubMS = (int)msg[4];
	towSubMS += ((int)msg[5]) << 8;
	towSubMS += ((int)msg[6]) << 16;
	towSubMS += ((int)msg[7]) << 24;
    //int itow = towMS / 1000;
	// quantization error
	int32_t qErr = (int)msg[8];
	qErr += ((int)msg[9]) << 8;
	qErr += ((int)msg[10]) << 16;
	qErr += ((int)msg[11]) << 24;
    //int quantErr = qErr;
    //mutex.lock();
    emit gpsPropertyUpdatedInt32(qErr,TPQuantErr.updateAge(),'e');
	TPQuantErr = qErr;
    //mutex.unlock();
	// week number
	uint16_t week = (int)msg[12];
	week += ((int)msg[13]) << 8;
    //int weekNr = week;
	// flags
	uint8_t flags = msg[14];
	// ref info
	uint8_t refInfo = msg[15];

	double sr = towMS / 1000.;
	sr = sr - towMS / 1000;

	//   cout<<"0d 01 "<<dec<<weekNr<<" "<<towMS/1000<<" "<<(long int)(sr*1e9+towSubMS+0.5)<<" "<<qErr<<flush<<endl;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-TIM-TP message:" << endl;
        tempStream << " tow s            : " << dec << towMS / 1000. << " s" << endl;
        tempStream << " tow sub s        : " << dec << towSubMS << " = " << (long int)(sr*1e9 + towSubMS + 0.5) << " ns" << endl;
        tempStream << " quantization err : " << dec << qErr << " ps" << endl;
        tempStream << " week nr          : " << dec << week << endl;
        tempStream << " *flags            : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << "  time base     : " << string(((flags & 1) ? "UTC" : "GNSS")) << endl;
        tempStream << "  UTC available : " << string((flags & 2) ? "yes" : "no") << endl;
        tempStream << "  (T)RAIM info  : " << (int)((flags & 0x0c) >> 2) << endl;
        tempStream << " *refInfo          : ";
        for (int i = 7; i >= 0; i--) if (refInfo & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
		string gnssRef;
		switch (refInfo & 0x0f) {
		case 0:
			gnssRef = "GPS";
			break;
		case 1:
			gnssRef = "GLONASS";
			break;
		case 2:
			gnssRef = "BeiDou";
			break;
		default:
			gnssRef = "unknown";
		}
        tempStream << "  GNSS reference : " << gnssRef << endl;
		string utcStd;
		switch ((refInfo & 0xf0) >> 4) {
		case 0:
			utcStd = "n/a";
			break;
		case 1:
			utcStd = "CRL";
			break;
		case 2:
			utcStd = "NIST";
			break;
		case 3:
			utcStd = "USNO";
			break;
		case 4:
			utcStd = "BIPM";
			break;
		case 5:
			utcStd = "EU";
			break;
		case 6:
			utcStd = "SU";
			break;
		default:
			utcStd = "unknown";
		}
        tempStream << "  UTC standard  : " << utcStd << endl;
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
	return true;
}

bool Ublox::UBXTimTM2(const std::string& msg, int verbose)
{
	// parse all fields
	// channel
	uint8_t ch = msg[0];
	// flags
	uint8_t flags = msg[1];
	// rising edge counter
	uint16_t count = (int)msg[2];
	count += ((int)msg[3]) << 8;
	// week number of last rising edge
	uint16_t wnR = (int)msg[4];
	wnR += ((int)msg[5]) << 8;
	// week number of last falling edge
	uint16_t wnF = (int)msg[6];
	wnF += ((int)msg[7]) << 8;
	// time of week of rising edge, ms
	uint32_t towMsR = (int)msg[8];
	towMsR += ((int)msg[9]) << 8;
	towMsR += ((int)msg[10]) << 16;
	towMsR += ((int)msg[11]) << 24;
	// time of week of rising edge, sub ms
	uint32_t towSubMsR = (int)msg[12];
	towSubMsR += ((int)msg[13]) << 8;
	towSubMsR += ((int)msg[14]) << 16;
	towSubMsR += ((int)msg[15]) << 24;
	// time of week of falling edge, ms
	uint32_t towMsF = (int)msg[16];
	towMsF += ((int)msg[17]) << 8;
	towMsF += ((int)msg[18]) << 16;
	towMsF += ((int)msg[19]) << 24;
	// time of week of falling edge, sub ms
	uint32_t towSubMsF = (int)msg[20];
	towSubMsF += ((int)msg[21]) << 8;
	towSubMsF += ((int)msg[22]) << 16;
	towSubMsF += ((int)msg[23]) << 24;
	// accuracy estimate
	uint32_t accEst = (int)msg[24];
	accEst += ((int)msg[25]) << 8;
	accEst += ((int)msg[26]) << 16;
	accEst += ((int)msg[27]) << 24;

    //mutex.lock();
    emit gpsPropertyUpdatedUint32(accEst, timeAccuracy.updateAge(), 'a');
    timeAccuracy = accEst;
    timeAccuracy.lastUpdate = std::chrono::system_clock::now();
    //mutex.unlock();

	double sr = towMsR / 1000.;
	sr = sr - towMsR / 1000;
	//  sr*=1000.;
	double sf = towMsF / 1000.;
	sf = sf - towMsF / 1000;
	//  sf*=1000.;
	//  cout<<"sr="<<sr<<" sf="<<sf<<endl;

	  // meaning of columns:
	  // 0d 03 - signature of TIM-TM2 message
	  // ch, week nr, second in current week (rising), ns of timestamp in current second (rising),
	  // second in current week (falling), ns of timestamp in current second (falling),
	  // accuracy (ns), rising edge counter, rising/falling edge (1/0), time valid (GNSS fix)
	//   cout<<"0d 03 "<<dec<<(int)ch<<" "<<wnR<<" "<<towMsR/1000<<" "<<(long int)(sr*1e9+towSubMsR)<<" "<<towMsF/1000<<" "<<(long int)(sf*1e9+towSubMsF)<<" "<<accEst<<" "<<count<<" "<<string((flags&0x80)?"1":"0")<<" "<<string((flags&0x40)?"1":"0")<<flush;
	//   cout<<endl;
	//  cout<<flush;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-TimTM2 message:" << endl;
        tempStream << " channel         : " << dec << (int)ch << endl;
        tempStream << " rising edge ctr : " << dec << count << endl;
        tempStream << " * last rising edge:" << endl;
        tempStream << "    week nr        : " << dec << wnR << endl;
        tempStream << "    tow s          : " << dec << towMsR / 1000. << " s" << endl;
        tempStream << "    tow sub s     : " << dec << towSubMsR << " = " << (long int)(sr*1e9 + towSubMsR) << " ns" << endl;
        tempStream << " * last falling edge:" << endl;
        tempStream << "    week nr        : " << dec << wnF << endl;
        tempStream << "    tow s          : " << dec << towMsF / 1000. << " s" << endl;
        tempStream << "    tow sub s      : " << dec << towSubMsF << " = " << (long int)(sf*1e9 + towSubMsF) << " ns" << endl;
        tempStream << " accuracy est      : " << dec << accEst << " ns" << endl;
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << "   mode                 : " << string((flags & 1) ? "single" : "running") << endl;
        tempStream << "   run                  : " << string((flags & 2) ? "armed" : "stopped") << endl;
        tempStream << "   new rising edge      : " << string((flags & 0x80) ? "yes" : "no") << endl;
        tempStream << "   new falling edge     : " << string((flags & 0x04) ? "yes" : "no") << endl;
        tempStream << "   UTC available        : " << string((flags & 0x20) ? "yes" : "no") << endl;
        tempStream << "   time valid (GNSS fix): " << string((flags & 0x40) ? "yes" : "no") << endl;
		string timeBase;
		switch ((flags & 0x18) >> 3) {
		case 0:
			timeBase = "receiver time";
			break;
		case 1:
			timeBase = "GNSS";
			break;
		case 2:
			timeBase = "UTC";
			break;
		default:
			timeBase = "unknown";
		}
        tempStream << "   time base            : " << timeBase;
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}

    struct timespec ts_r = unixtime_from_gps(wnR, towMsR / 1000, (long int)(sr*1e9 + towSubMsR)/*, this->leapSeconds()*/);
    struct timespec ts_f = unixtime_from_gps(wnF, towMsF / 1000, (long int)(sf*1e9 + towSubMsF)/*, this->leapSeconds()*/);

	struct gpsTimestamp ts;
	ts.rising_time = ts_r;
	ts.falling_time = ts_f;
	ts.valid = (flags & 0x40);
	ts.channel = ch;

	//   fTimestamps.push(ts);
    ts.counter = count;
	ts.accuracy_ns = accEst;

    ts.rising = ts.falling = false;
	if (flags & 0x80) {
		// new rising edge detected
		ts.rising = true;


	} if (flags & 0x04) {
		// new falling edge detected
		ts.falling = true;

	}
    //mutex.lock();
    emit gpsPropertyUpdatedUint32(count, eventCounter.updateAge(),'c');
    eventCounter = count;
	fTimestamps.push(ts);
    //mutex.unlock();

	return true;
}

std::vector<GnssSatellite> Ublox::UBXNavSat(bool allSats, int verbose)
{
	std::string answer;
	std::vector<GnssSatellite> satList;
	// UBX-NAV-SAT: satellite information
    bool ok = pollUBX(MSG_NAV_SAT, answer, MSGTIMEOUT, verbose);
	//  bool ok=pollUBX(0x0d, 0x03, 0, answer, MSGTIMEOUT);
	if (!ok) return satList;

    return UBXNavSat(answer, allSats, verbose);

	// parse all fields
	// GPS time of week
	uint32_t iTOW = (int)answer[0];
	iTOW += ((int)answer[1]) << 8;
	iTOW += ((int)answer[2]) << 16;
	iTOW += ((int)answer[3]) << 24;
	// version
	uint8_t version = answer[4];
	uint8_t numSvs = answer[5];

	int N = (answer.size() - 8) / 12;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-NAV-SAT message:" << endl;
        tempStream << " iTOW          : " << dec << iTOW / 1000 << " s" << endl;
        tempStream << " version       : " << dec << (int)version << endl;
        tempStream << " Nr of sats    : " << (int)numSvs << "  (nr of sections=" << N << ")" << endl;
        tempStream << "   Sat Data :";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
	int goodSats = 0;
	for (int i = 0; i < N; i++) {
		GnssSatellite sat(answer.substr(8 + 12 * i, 12));
		if (sat.getCnr() > 0) goodSats++;
		satList.push_back(sat);
	}
	if (!allSats) {
		sort(satList.begin(), satList.end(), GnssSatellite::sortByCnr);
		while (satList.back().getCnr() == 0 && satList.size() > 0) satList.pop_back();
	}

    if (verbose) {
        std::string temp;
		GnssSatellite::PrintHeader(true);
		for (vector<GnssSatellite>::iterator it = satList.begin(); it != satList.end(); it++) {
			it->Print(distance(satList.begin(), it), false);
		}
        std::stringstream tempStream;
        tempStream << "   --------------------------------------------------------------------\n";
        if (verbose > 1){
            tempStream << " Nr of avail sats : " << goodSats;
        }
        emit toConsole(QString::fromStdString(tempStream.str()));
    }
	return satList;
}

std::vector<GnssSatellite> Ublox::UBXNavSat(const std::string& msg, bool allSats, int verbose)
{
	std::vector<GnssSatellite> satList;
	// UBX-NAV-SAT: satellite information
	// parse all fields
	// GPS time of week
	uint32_t iTOW = (int)msg[0];
	iTOW += ((int)msg[1]) << 8;
	iTOW += ((int)msg[2]) << 16;
	iTOW += ((int)msg[3]) << 24;
	// version
	uint8_t version = msg[4];
	uint8_t numSvs = msg[5];

	int N = (msg.size() - 8) / 12;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << setfill(' ') << setw(3);
        tempStream << "*** UBX-NAV-SAT message:" << endl;
        tempStream << " iTOW          : " << dec << iTOW / 1000 << " s" << endl;
        tempStream << " version       : " << dec << (int)version << endl;
        tempStream << " Nr of sats    : " << (int)numSvs << "  (nr of sections=" << N << ")" << endl;
        tempStream << "   Sat Data :";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
    uint8_t goodSats = 0;
	for (int i = 0; i < N; i++) {
		GnssSatellite sat(msg.substr(8 + 12 * i, 12));
		if (sat.getCnr() > 0) goodSats++;
		satList.push_back(sat);
	}
	if (!allSats) {
		sort(satList.begin(), satList.end(), GnssSatellite::sortByCnr);
		while (satList.back().getCnr() == 0 && satList.size() > 0) satList.pop_back();
	}

	//  nrSats = satList.size();
    //mutex.lock();
    emit gpsPropertyUpdatedUint8(goodSats,nrSats.updateAge(),'s');
    nrSats = goodSats;
    //mutex.unlock();

    if (verbose) {
        std::string temp;
		GnssSatellite::PrintHeader(true);
		for (vector<GnssSatellite>::iterator it = satList.begin(); it != satList.end(); it++) {
			it->Print(distance(satList.begin(), it), false);
		}
        std::stringstream tempStream;
        tempStream << "   --------------------------------------------------------------------\n";
        if (verbose > 1){
            tempStream << " Nr of avail sats : " << goodSats;
        }
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
	return satList;
}


bool Ublox::UBXCfgGNSS(int verbose)
{
	std::string answer;
	// UBX-CFG-GNSS: GNSS configuration
    bool ok = pollUBX(MSG_CFG_GNSS, answer, MSGTIMEOUT, verbose);
	if (!ok) return false;
	// parse all fields
	// version
	uint8_t version = answer[0];
	uint8_t numTrkChHw = answer[1];
	uint8_t numTrkChUse = answer[2];
	uint8_t numConfigBlocks = answer[3];

	int N = (answer.size() - 4) / 8;

    //  if (verbose>1)
	{
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX CFG-GNSS message:" << endl;
        tempStream << " version                    : " << dec << (int)version << endl;
        tempStream << " nr of hw tracking channels : " << dec << (int)numTrkChHw << endl;
        tempStream << " nr of channels in use      : " << dec << (int)numTrkChUse << endl;
        tempStream << " Nr of config blocks        : " << (int)numConfigBlocks
                   << "  (nr of sections=" << N << ")";
        tempStream << "  Config Data :";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
	for (int i = 0; i < N; i++) {
		uint8_t gnssID = answer[4 + 8 * i];
		uint8_t resTrkCh = answer[5 + 8 * i];
		uint8_t maxTrkCh = answer[6 + 8 * i];
		uint32_t flags = answer[8 + 8 * i];
		flags |= (int)answer[9 + 8 * i] << 8;
		flags |= (int)answer[10 + 8 * i] << 16;
		flags |= (int)answer[11 + 8 * i] << 24;
        //    if (verbose>1)
		{
            std::stringstream tempStream;
            //std::string temp;
            //mutex.lock();
            tempStream << "   " << i << ":   GNSS name : "
                       << GnssSatellite::GNSS_ID_STRING[gnssID] << endl;
            //mutex.unlock();
            tempStream << "      reserved (min) tracking channels  : "
                       << dec << (int)resTrkCh << endl;
            tempStream << "      max nr of tracking channels used : "
                       << dec << (int)maxTrkCh << endl;
            tempStream << "      flags  : " << hex << (int)flags;
            //tempStream >> temp;
            emit toConsole(QString::fromStdString(tempStream.str()));
		}
	}
	return true;
}

bool Ublox::UBXCfgNav5(int verbose)
{
	std::string answer;
	// UBX CFG-NAV5: satellite information
    bool ok = pollUBX(MSG_CFG_NAV5, answer, MSGTIMEOUT, verbose);
	if (!ok) return false;
	// parse all fields
	// version
	uint16_t mask = answer[0];
	mask |= (int)answer[1] << 8;
	uint8_t dynModel = answer[2];
	uint8_t fixMode = answer[3];
	int32_t fixedAlt = answer[4];
	fixedAlt |= (int)answer[5] << 8;
	fixedAlt |= (int)answer[6] << 16;
	fixedAlt |= (int)answer[7] << 24;
	uint32_t fixedAltVar = answer[8];
	fixedAltVar |= (int)answer[9] << 8;
	fixedAltVar |= (int)answer[10] << 16;
	fixedAltVar |= (int)answer[11] << 24;
	int8_t minElev = answer[12];


    //  if (verbose>1)
	{
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX CFG-NAV5 message:" << endl;
        tempStream << " mask               : " << hex << (int)mask << endl;
        tempStream << " dynamic model used : " << dec << (int)dynModel << endl;
        tempStream << " fixMode            : " << dec << (int)fixMode << endl;
        tempStream << " fixed Alt          : " << (double)fixedAlt*0.01 << " m" << endl;
        tempStream << " fixed Alt Var      : " << (double)fixedAltVar*0.0001 << " m^2" << endl;
        tempStream << " min elevation      : " << dec << (int)minElev << " deg";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
	return true;
}

std::vector<std::string> Ublox::UBXMonVer(int verbose)
{
	std::string answer;
	std::vector<std::string> result;
	// UBX CFG-NAV5: satellite information
    bool ok = pollUBX(MSG_MON_VER, answer, MSGTIMEOUT,verbose);
	if (!ok) return result;
	// parse all fields
	// version
	cout << "*** UBX MON-VER message:" << endl;
	//  cout<<answer<<endl;
    std::string::size_type i = 0;
	while (i != std::string::npos && i < answer.size()) {
		std::string s = answer.substr(i, answer.find((char)0x00, i + 1) - i + 1);
		while (s.size() && s[0] == 0x00) {
			s.erase(0, 1);
		}
		if (s.size()) {
			result.push_back(s);
            emit toConsole(QString::fromStdString(s));
		}
		i = answer.find((char)0x00, i + 1);
	}
	return result;
}


void Ublox::UBXNavClock(const std::string& msg, int verbose)
{
	// parse all fields
	// GPS time of week
	uint32_t iTOW = (int)msg[0];
	iTOW += ((int)msg[1]) << 8;
	iTOW += ((int)msg[2]) << 16;
	iTOW += ((int)msg[3]) << 24;
    //uint32_t itow = iTOW / 1000;
	// clock bias
    if (verbose > 3) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "clkB[0]=" << hex << (int)msg[4] << endl;
        tempStream << "clkB[1]=" << hex << (int)msg[5] << endl;
        tempStream << "clkB[2]=" << hex << (int)msg[6] << endl;
        tempStream << "clkB[3]=" << hex << (int)msg[7];
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
	int32_t clkB = (int)msg[4];
	clkB += ((int)msg[5]) << 8;
	clkB += ((int)msg[6]) << 16;
	clkB += ((int)msg[7]) << 24;
    //int32_t bias = clkB;
	// clock drift
	int32_t clkD = (int)msg[8];
	clkD += ((int)msg[9]) << 8;
	clkD += ((int)msg[10]) << 16;
	clkD += ((int)msg[11]) << 24;
    //int32_t drift = clkD;
    //mutex.lock();
    emit gpsPropertyUpdatedInt32(clkD,clkDrift.updateAge(),'d');
    emit gpsPropertyUpdatedInt32(clkB,clkBias.updateAge(),'b');
    clkDrift = clkD;
    clkBias = clkB;
    //mutex.unlock();
    // time accuracy estimate
	uint32_t tAcc = (int)msg[12];
	tAcc += ((int)msg[13]) << 8;
	tAcc += ((int)msg[14]) << 16;
	tAcc += ((int)msg[15]) << 24;
    //uint32_t tAccuracy = tAcc;
	//timeAccuracy = tAcc;
	// freq accuracy estimate
	uint32_t fAcc = (int)msg[16];
	fAcc += ((int)msg[17]) << 8;
	fAcc += ((int)msg[18]) << 16;
	fAcc += ((int)msg[19]) << 24;
    //uint32_t fAccuracy = fAcc;

	// meaning of columns:
	// 01 22 - signature of NAV-CLOCK message
	// second in current week (s), clock bias (ns), clock drift (ns/s), time accuracy (ns), freq accuracy (ps/s)
  //   cout<<"01 22 "<<dec<<iTOW/1000<<" "<<clkB<<" "<<clkD<<" "<<tAcc<<" "<<fAcc<<flush;
  //   cout<<endl;


    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-NAV-CLOCK message:" << endl;
        tempStream << " iTOW          : " << dec << iTOW / 1000 << " s" << endl;
        tempStream << " clock bias    : " << dec << clkB << " ns" << endl;
        tempStream << " clock drift   : " << dec << clkD << " ns/s" << endl;
        tempStream << " time accuracy : " << dec << tAcc << " ns" << endl;
        tempStream << " freq accuracy : " << dec << fAcc << " ps/s";
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
}


void Ublox::UBXNavTimeGPS(const std::string& msg, int verbose)
{
	// parse all fields
	// GPS time of week
	uint32_t iTOW = (int)msg[0];
	iTOW += ((int)msg[1]) << 8;
	iTOW += ((int)msg[2]) << 16;
	iTOW += ((int)msg[3]) << 24;

	int32_t fTOW = (int)msg[4];
	fTOW += ((int)msg[5]) << 8;
	fTOW += ((int)msg[6]) << 16;
	fTOW += ((int)msg[7]) << 24;

	//  int32_t ftow=iTOW/1000;

	uint16_t wnR = (int)msg[8];
	wnR += ((int)msg[9]) << 8;

	int8_t leapS = (int)msg[10];
	uint8_t flags = (int)msg[11];

	// time accuracy estimate
	uint32_t tAcc = (int)msg[12];
	tAcc += ((int)msg[13]) << 8;
	tAcc += ((int)msg[14]) << 16;
	tAcc += ((int)msg[15]) << 24;
    //uint32_t tAccuracy = tAcc;


	double sr = iTOW / 1000.;
	sr = sr - iTOW / 1000;

	// meaning of columns:
	// 01 20 - signature of NAV-TIMEGPS message
	// week nr, second in current week, ns of timestamp in current second,
	// nr of leap seconds wrt UTC, accuracy (ns)
  //   cout<<"01 20 "<<dec<<wnR<<" "<<iTOW/1000<<" "<<(long int)(sr*1e9+fTOW)<<" "<<(int)leapS<<" "<<tAcc<<flush;
  //   cout<<endl;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-NAV-TIMEGPS message:" << endl;
        tempStream << " week nr       : " << dec << wnR << endl;
        tempStream << " iTOW          : " << dec << iTOW << " ms = " << iTOW / 1000 << " s" << endl;
        tempStream << " fTOW          : " << dec << fTOW << " = " << (long int)(sr*1e9 + fTOW) << " ns" << endl;
        tempStream << " leap seconds  : " << dec << (int)leapS << " s" << endl;
        tempStream << " time accuracy : " << dec << tAcc << " ns" << endl;
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << "   tow valid        : " << string((flags & 1) ? "yes" : "no") << endl;
        tempStream << "   week valid       : " << string((flags & 2) ? "yes" : "no") << endl;
        tempStream << "   leap sec valid   : " << string((flags & 4) ? "yes" : "no");
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
    //mutex.lock();
    emit gpsPropertyUpdatedUint32(tAcc, timeAccuracy.updateAge(), 'a');
    timeAccuracy = tAcc;
    timeAccuracy.lastUpdate = std::chrono::system_clock::now();
    if (flags & 4) { leapSeconds = leapS; }
    //mutex.unlock();

    struct timespec ts = unixtime_from_gps(wnR, iTOW / 1000, (long int)(sr*1e9 + fTOW)/*, this->leapSeconds()*/);
    std::stringstream tempStream;
    //std::string temp;
    tempStream << ts.tv_sec << '.' << ts.tv_nsec;
    //tempStream >> temp;
    emit toConsole(QString::fromStdString(tempStream.str()));
}

void Ublox::UBXNavTimeUTC(const std::string& msg, int verbose)
{
	// parse all fields
	// GPS time of week
	uint32_t iTOW = (int)msg[0];
	iTOW += ((int)msg[1]) << 8;
	iTOW += ((int)msg[2]) << 16;
	iTOW += ((int)msg[3]) << 24;

	// time accuracy estimate
	uint32_t tAcc = (int)msg[4];
	tAcc += ((int)msg[5]) << 8;
	tAcc += ((int)msg[6]) << 16;
	tAcc += ((int)msg[7]) << 24;
    //uint32_t tAccuracy = tAcc;
    //mutex.lock();
    emit gpsPropertyUpdatedUint32(tAcc, timeAccuracy.updateAge(), 'a');
    timeAccuracy = tAcc;
    timeAccuracy.lastUpdate = std::chrono::system_clock::now();
    //mutex.unlock();

	int32_t nano = (int)msg[8];
	nano += ((int)msg[9]) << 8;
	nano += ((int)msg[10]) << 16;
	nano += ((int)msg[11]) << 24;

	uint16_t year = (int)msg[12];
	year += ((int)msg[13]) << 8;

	uint16_t month = (int)msg[14];
	uint16_t day = (int)msg[15];
	uint16_t hour = (int)msg[16];
	uint16_t min = (int)msg[17];
	uint16_t sec = (int)msg[18];

	uint8_t flags = (int)msg[19];

	double sr = iTOW / 1000.;
	sr = sr - iTOW / 1000;

	// meaning of columns:
	// 01 21 - signature of NAV-TIMEUTC message
	// second in current week, year, month, day, hour, minute, seconds(+fraction)
	// accuracy (ns)
  //   cout<<"01 21 "<<dec<<iTOW/1000<<" "<<year<<" "<<month<<" "<<day<<" "<<hour<<" "<<min<<" "<<(double)(sec+nano*1e-9)<<" "<<tAcc<<flush;
  //   cout<<endl;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-NAV-TIMEUTC message:" << endl;
        tempStream << " iTOW           : " << dec << iTOW << " ms = " << iTOW / 1000 << " s" << endl;
        tempStream << " nano           : " << dec << nano << " ns" << endl;
        tempStream << " date y/m/d     : " << dec << (int)year << "/" << (int)month << "/" << (int)day << endl;
        tempStream << " UTC time h:m:s : " << dec << setw(2) << setfill('0') << hour << ":" << (int)min << ":" << (double)(sec + nano * 1e-9) << endl;
        tempStream << " time accuracy : " << dec << tAcc << " ns" << endl;
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << "   tow valid        : " << string((flags & 1) ? "yes" : "no") << endl;
        tempStream << "   week valid       : " << string((flags & 2) ? "yes" : "no") << endl;
        tempStream << "   UTC time valid   : " << string((flags & 4) ? "yes" : "no") << endl;
		string utcStd;
		switch ((flags & 0xf0) >> 4) {
		case 0:
			utcStd = "n/a";
			break;
		case 1:
			utcStd = "CRL";
			break;
		case 2:
			utcStd = "NIST";
			break;
		case 3:
			utcStd = "USNO";
			break;
		case 4:
			utcStd = "BIPM";
			break;
		case 5:
			utcStd = "EU";
			break;
		case 6:
			utcStd = "SU";
			break;
		case 7:
			utcStd = "NTSC";
			break;
		default:
			utcStd = "unknown";
		}
        tempStream << "   UTC standard  : " << utcStd;
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
}

void Ublox::UBXMonHW(const std::string& msg, int verbose)
{
	// parse all fields
	// noise
	uint16_t noisePerMS = (int)msg[16];
	noisePerMS += ((int)msg[17]) << 8;
	noise = noisePerMS;

	// agc
	uint16_t agcCnt = (int)msg[18];
	agcCnt += ((int)msg[19]) << 8;
	agc = agcCnt;

	uint8_t flags = (int)msg[22];

	// meaning of columns:
	// 01 21 - signature of NAV-TIMEUTC message
	// second in current week, year, month, day, hour, minute, seconds(+fraction)
	// accuracy (ns)
  //   cout<<"0a 09 "<<dec<<noisePerMS<<" "<<agcCnt<<" "<<flush;
  //   cout<<endl;

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "*** UBX-MON-HW message:" << endl;
        tempStream << " noise            : " << dec << noisePerMS << " dBc" << endl;
        tempStream << " agcCnt (0..8192) : " << dec << agcCnt << endl;
        tempStream << " flags             : ";
        for (int i = 7; i >= 0; i--) if (flags & 1 << i) tempStream << i; else tempStream << "-";
        tempStream << endl;
        tempStream << "   RTC calibrated   : " << string((flags & 1) ? "yes" : "no") << endl;
        tempStream << "   safe boot        : " << string((flags & 2) ? "yes" : "no") << endl;
        tempStream << "   jamming state    : " << (int)((flags & 0x0c) >> 2) << endl;
        tempStream << "   Xtal absent      : " << string((flags & 0x10) ? "yes" : "no");
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
}

void Ublox::UBXMonTx(const std::string& msg, int verbose)
{
	// parse all fields
	// nr bytes pending
	uint16_t pending[6];
	uint8_t usage[6];
	uint8_t peakUsage[6];
	uint8_t tUsage;
	uint8_t tPeakUsage;
    //uint8_t errors;

	for (int i = 0; i < 6; i++) {
		pending[i] = msg[2 * i];
		pending[i] |= (uint16_t)msg[2 * i + 1] << 8;
		usage[i] = msg[i + 12];
		peakUsage[i] = msg[i + 18];
	}

	tUsage = msg[24];
	tPeakUsage = msg[25];
    //errors = msg[26];

	// meaning of columns:
	// 01 21 - signature of NAV-TIMEUTC message
	// second in current week, year, month, day, hour, minute, seconds(+fraction)
	// accuracy (ns)
  //   cout<<"0a 09 "<<dec<<noisePerMS<<" "<<agcCnt<<" "<<flush;
  //   cout<<endl;
    //mutex.lock();
    emit gpsPropertyUpdatedUint8(tUsage,txBufUsage.updateAge(),'b');
    emit gpsPropertyUpdatedUint8(tPeakUsage, txBufPeakUsage.updateAge(), 'p');
	txBufUsage = tUsage;
	txBufPeakUsage = tPeakUsage;
    //mutex.unlock();

    if (verbose > 1) {
        std::stringstream tempStream;
        //std::string temp;
        tempStream << setfill(' ') << setw(3);
        tempStream << "*** UBX-MON-TXBUF message:" << endl;
        tempStream << " global TX buf usage      : " << dec << (int)tUsage << " %" << endl;
        tempStream << " global TX buf peak usage : " << dec << (int)tPeakUsage << " %" << endl;
        tempStream << " TX buf usage for target      : ";
		for (int i = 0; i < 6; i++) {
            tempStream << "    (" << dec << i << ") " << setw(3) << (int)usage[i];
		}
        tempStream << endl;
        tempStream << " TX buf peak usage for target : ";
		for (int i = 0; i < 6; i++) {
            tempStream << "    (" << dec << i << ") " << setw(3) << (int)peakUsage[i];
		}
        tempStream << endl;
        tempStream << " TX bytes pending for target  : ";
		for (int i = 0; i < 6; i++) {
            tempStream << "    (" << dec << i << ") " << setw(3) << pending[i];
		}
        tempStream << setw(1) << setfill(' ');
        //tempStream >> temp;
        emit toConsole(QString::fromStdString(tempStream.str()));
	}
}

void Ublox::UBXSetCfgMsg(uint16_t msgID, uint8_t port, uint8_t rate, int verbose)
{
    if (verbose>2){
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "Ublox UBXsetCfgMsg reporting from thread " << this->thread();
        emit toConsole(QString::fromStdString(tempStream.str()));
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
	if (waitAck(MSG_CFG_MSG, 12000)) {
		emit toConsole("Set CFG successful");
	}
	else {
        emit UBXCfgError("Set CFG timeout");
	}
}

void Ublox::UBXSetCfgMsg(uint8_t classID, uint8_t messageID, uint8_t port, uint8_t rate, int verbose)
{
    uint16_t msgID = (messageID + (uint16_t)classID) << 8;
    UBXSetCfgMsg(msgID, port, rate, verbose);
}

void Ublox::UBXSetCfgRate(uint8_t measRate, uint8_t navRate, int verbose)
{
    if (verbose>2){
        QTextStream str;
        std::stringstream tempStream;
        //std::string temp;
        tempStream << "Ublox UBXsetCfgRate reporting from thread " << this->thread();
        emit toConsole(QString::fromStdString(tempStream.str()));
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
	if (waitAck(MSG_CFG_RATE, 10000))
	{
        emit toConsole("Set CFG successful");
	}
	else {
        emit UBXCfgError("Set CFG timeout");
	}
}

gpsTimestamp Ublox::getEventFIFOEntry() {
    //mutex.lock();
    gpsTimestamp ts = fTimestamps.front();
    fTimestamps.pop();
    //mutex.unlock();
    return ts;
}
