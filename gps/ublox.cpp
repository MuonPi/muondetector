#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <iterator>     // std::distance
#include <thread>
#include <chrono>
//#include <SerialStream.h>
//#include <SerialPort.h>
#include "serial.h"

#include "ublox.h"

#define deg "\u00B0"
#define DEFAULT_VERBOSITY 0
#define MAX_MESSAGE_BUFSIZE 1000

//using namespace LibSerial;
using namespace std;

const int	TIMEOUT  = 100;
const bool 	BLOCKING = true;
const int	MSGTIMEOUT  = 1500;

const string GnssSatellite::GNSS_ID_STRING[] = {" GPS","SBAS"," GAL","BEID","IMES","QZSS","GLNS" };

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

static time_t
t_from_rtc(struct tm *stm) {
  bool rtc_on_utc = true;
  
  struct tm temp1, temp2;
  long diff;
  time_t t1, t2;

  temp1 = *stm;
  temp1.tm_isdst = 0;
  
  t1 = mktime(&temp1);
  if (rtc_on_utc) {
    temp2 = *gmtime(&t1);
  } else {
    temp2 = *localtime(&t1);
  }
  
  temp2.tm_isdst = 0;
  t2 = mktime(&temp2);
  diff = t2 - t1;

  if (t1 - diff == -1)
    //DEBUG_LOG(LOGF_RtcLinux, "Could not convert RTC time");
    printf("Could not convert RTC time");
  return t1 - diff;
}


struct timespec unixtime_from_gps(int week_nr, long int s_of_week, long int ns, int leap_seconds) {
  struct tm time;
  time.tm_year=80;
  time.tm_mon=0;
  time.tm_mday=6;
  time.tm_isdst=-1;
  time.tm_hour=0;
  time.tm_min=0;
  time.tm_sec=0;
  
  int GpsCycle = 0;

  long GpsDays = 1024*7;
  time.tm_mday+=GpsDays;
  mktime(&time);
  GpsDays = ((GpsCycle * 1024) + (week_nr-1024)) * 7 + (s_of_week / 86400L);
//  time.tm_mday+=10000;
  time.tm_mday+=GpsDays;
  mktime(&time);
  
  long int sod = s_of_week - ((long int)(s_of_week / 86400L))*86400L;
//  printf("s of d: %ld\n",msod/1000);
  time.tm_hour=sod/3600;
  time.tm_min=sod/60-time.tm_hour*60;
  time.tm_sec=sod-time.tm_hour*3600-time.tm_min*60/*+leap_seconds*/;
  mktime(&time);
  
  
  t_from_rtc(&time);
  
  time_t secs = t_from_rtc(&time);
   
//   printf("GPS time: %s",asctime(&time));

  struct timespec ts;
  ts.tv_sec = secs;
  ts.tv_nsec = ns;
  
//   cout<<secs<<"."<<setw(6)<<setfill('0')<<ns<<" "<<setfill(' ')<<endl;
  
  return ts;
}


bool GnssSatellite::sortByCnr(const GnssSatellite &sat1, const GnssSatellite &sat2)
{ return sat1.getCnr() > sat2.getCnr(); }




GnssSatellite::GnssSatellite(const std::string& ubxNavSatSubMessage) {
  const string mess(ubxNavSatSubMessage);
  
  fGnssId=mess[0];
  fSatId=mess[1];
  fCnr=mess[2];
  fElev=mess[3];
  fAzim=mess[4];
  fAzim+=mess[5]<<8;
  int16_t prRes=mess[6];
  prRes+=mess[7]<<8;
  fPrRes = prRes/10.;
  
  fFlags = (int)mess[8];
  fFlags+=((int)mess[9])<<8;
  fFlags+=((int)mess[10])<<16;
  fFlags+=((int)mess[11])<<24;
  
  fQuality = (int)(fFlags & 0x07);
  if (fFlags & 0x08) fUsed=true; else fUsed=false;
  fHealth = (int)(fFlags>>4 & 0x03);
  fOrbitSource = (fFlags>>8 & 0x07);
  fSmoothed = (fFlags & 0x80);
  fDiffCorr = (fFlags & 0x40);
}

void GnssSatellite::PrintHeader(bool wIndex)
{
  if (wIndex) {
    cout<<"   ----------------------------------------------------------------------------------"<<endl;
    cout<<"   Nr   Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr"<<endl;
    cout<<"   ----------------------------------------------------------------------------------"<<endl;
  } else {
    cout<<"   -----------------------------------------------------------------"<<endl;
    cout<<"    Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr"<<endl;
    cout<<"   -----------------------------------------------------------------"<<endl;
  }
}


void GnssSatellite::Print(bool wHeader)
{
  if (wHeader) {
    cout<<"   ------------------------------------------------------------------------------"<<endl;
    cout<<"    Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr"<<endl;
    cout<<"   ------------------------------------------------------------------------------"<<endl;
  }
  cout<<"   "<<dec<<"  "<<GNSS_ID_STRING[(int)fGnssId]<<"   "<<setw(3)<<(int)fSatId<<"    ";
//    cout<<setfill(' ');
  cout<<setw(3)<<(int)fCnr<<"      " <<setw(3)<<(int)fElev<<"       "<<setw(3)<<(int)fAzim;
  cout<<"   "<<setw(6)<<fPrRes<<"    "<<fQuality<<"   "<<string((fUsed)?"Y":"N");
  cout<<"    "<<fHealth<<"   "<<fOrbitSource<<"   "<<(int)fSmoothed<<"    "<<(int)fDiffCorr;
  cout<<endl;
}

void GnssSatellite::Print(int index, bool wHeader)
{
  if (wHeader) {
    cout<<"   ----------------------------------------------------------------------------------"<<endl;
    cout<<"   Nr   Sys    ID   S/N(dB)  El(deg)  Az(deg)  Res(m) Qlty Use Hlth Src Smth DiffCorr"<<endl;
    cout<<"   ----------------------------------------------------------------------------------"<<endl;
  }
  cout<<"   "<<dec<<setw(2)<<index+1<<"  "<<GNSS_ID_STRING[(int)fGnssId]<<"   "<<setw(3)<<(int)fSatId<<"    ";
//    cout<<setfill(' ');
  cout<<setw(3)<<(int)fCnr<<"      " <<setw(3)<<(int)fElev<<"       "<<setw(3)<<(int)fAzim;
  cout<<"   "<<setw(6)<<fPrRes<<"    "<<fQuality<<"   "<<string((fUsed)?"Y":"N");
  cout<<"    "<<fHealth<<"   "<<fOrbitSource<<"   "<<(int)fSmoothed<<"    "<<(int)fDiffCorr;;
  cout<<endl;
}

//972(2)26714457
//6544262211
//0544262211
//0097226714457
//02


void Ublox::eventLoop()
{
  uint16_t msgID = 0;
  
  uint8_t classID = 0;
  uint8_t messageID = 0;

  std::string buf="";
  std::string buffer="";
  unsigned long iter=0;
  while (fActiveLoop) {
    int n=0;
    std::string answer="";
    std::string buf="";

//     while (iter<MSGTIMEOUT && !answer.size())
    char firstbyte;
    n=ReadBuffer(buf);
//     while (!_port->isDataAvailable(firstbyte))
    while (!buf.size() && fActiveLoop) { 
      if (fVerbose>3) cout<<"no data...sleep for 50ms"<<endl<<flush;
      std::this_thread::yield();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      n=ReadBuffer(buf);
      //usleep(20000);
    }
    //cout<<"first byte="<<hex<<(int)firstbyte<<endl;
    //buffer+=firstbyte;
    //n=ReadBuffer(buf)+1;
//     cout<<"there's data... n="<<dec<<(int)n<<" size="<<buf.size()<<endl;
    buffer+=buf;
    //iter++;
    UbxMessage message;
    answer=scanUnknownMessage(buffer,message.msgID);
    int i=0;
    while (answer.size() && fActiveLoop) {
//     cout<<"there's answer..."<<++i<<endl;
      message.data=answer;
//       if (answer.size()) {
      std::lock_guard<std::mutex> lock(fTimestampsMutex);	
      fMessageBuffer.push_back(message);
      processMessage(message);
      if (fVerbose>2) {
// 	cout<<"received UBX message "<<hex<<(int)message.classID<<" "<<hex<<(int)message.messageID<<" : ";
	cout<<"received UBX message "<<"0x"<<hex<<setw(4)<<setfill('0')<<message.msgID<<" : ";
	for (int i=0; i<answer.size(); i++) cout<<hex<<setw(2)<<setfill('0')<<(int)answer[i]<<" ";
	cout<<endl;
      }
      answer=scanUnknownMessage(buffer,message.msgID);
      if (fMessageBuffer.size()>MAX_MESSAGE_BUFSIZE) fMessageBuffer.pop_front();				
      if (fVerbose>3) cout<<"message buffer size: "<<fMessageBuffer.size()<<endl;
//     }
    }
//     while (fMessageBuffer.size()>MAX_MESSAGE_BUFSIZE) fMessageBuffer.erase(fMessageBuffer.begin());
    std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  if (fVerbose>2) cout<<"finishing read thread"<<endl;
}

void Ublox::processMessage(const UbxMessage& msg)
{

  uint8_t classID = (msg.msgID & 0xff00) >> 8;
  uint8_t messageID = msg.msgID & 0xff;
  
  std::vector<GnssSatellite> sats;

  switch (classID) {
	case 0x01 : // UBX-NAV 
		switch (messageID) {
			case 0x20: 
				if (fVerbose) {
					cout<<"received UBX-NAV-TIMEGPS message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
				UBXNavTimeGPS(msg.data);
				break;
			case 0x21: 
				if (fVerbose) {
					cout<<"received UBX-NAV-TIMEUTC message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
				UBXNavTimeUTC(msg.data);
				break;
			case 0x22: 
				if (fVerbose) {
					cout<<"received UBX-NAV-CLOCK message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
				UBXNavClock(msg.data);
				break;
			case 0x35:
				sats = UBXNavSat(msg.data, true);
				if (fVerbose) {
					cout<<"received UBX-NAV-SAT message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
				satList = sats;
				//break;
//				cout<<"Satellite List: "<<sats.size()<<" sats received"<<endl;
// 				GnssSatellite::PrintHeader(true);
// 				for (int i=0; i<sats.size(); i++) sats[i].Print(i, false);
				break;
			default:
				if (fVerbose) {
					cout<<"received unhandled UBX-NAV message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
		}
		break;
	case 0x02 : // UBX-RXM
		break;
	case 0x05 : // UBX-ACK
		break;
	case 0x0b : // UBX-AID
		break;
	case 0x06 : // UBX-CFG
		switch (messageID) {
			default:
				if (fVerbose) {
					cout<<"received unhandled UBX-CFG message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
		}
		break;
	case 0x10 : // UBX-ESF
		break;
	case 0x28 : // UBX-HNR
		break;
	case 0x04 : // UBX-INF
		switch (messageID) {
			default:
				if (fVerbose) {
					cout<<"received unhandled UBX-INF message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
		}
		break;
	case 0x21 : // UBX-LOG
		break;
	case 0x13 : // UBX-MGA
		break;
	case 0x0a : // UBX-MON
		switch (messageID) {
			case 0x08: 
				if (fVerbose) {
					cout<<"received UBX-MON-TXBUF message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
				UBXMonTx(msg.data);
				break;
			case 0x09: 
				if (fVerbose) {
					cout<<"received UBX-MON-HW message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
				UBXMonHW(msg.data);
				break;
			default:
				if (fVerbose) {
					cout<<"received unhandled UBX-MON message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
		}
		break;
	case 0x27 : // UBX-SEC
		break;
	case 0x0d : // UBX-TIM
		switch (messageID) {
			case 0x01: // UBX-TIM-TP
				if (fVerbose) {
					cout<<"received UBX-TIM-TP message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
				UBXTimTP(msg.data);
				break;
			case 0x03: // UBX-TIM-TM2
				if (fVerbose) {
					cout<<"received UBX-TIM-TM2 message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
				UBXTimTM2(msg.data);
				break;
			default:
				if (fVerbose) {
					cout<<"received unhandled UBX-TIM message ("<<hex<<(int)classID<<" "<<hex<<(int)messageID<<")"<<endl;
				}
		}
		break;
	case 0x09 : // UBX-UPD
		break;
	default: break;
  }
  cout<<dec;
}

Ublox::Ublox()
{
   _portName="/dev/ttyS0";
   _port= new Serial(_portName,9600,8,1,4096,Serial::NONE,BLOCKING);
   fVerbose=DEFAULT_VERBOSITY;
   fThread = 0;
}

Ublox::Ublox(const std::string& serialPortName)
{
   _portName=serialPortName;
//   _port= new Serial(_portName,9600);
   _port= new Serial(_portName,9600,8,1,4096,Serial::NONE,BLOCKING);
   fVerbose=DEFAULT_VERBOSITY;
   fThread = 0;
}

Ublox::Ublox(const std::string& serialPortName, int baudRate)
{
   _portName=serialPortName;
   _port= new Serial(_portName,baudRate,8,1,4096,Serial::NONE,BLOCKING);
   fVerbose=DEFAULT_VERBOSITY;
   fThread = 0;
}

Ublox::~Ublox()
{
   Disconnect();
   delete _port;
}

bool Ublox::Connect()
{
   if (!_port->isOpen())
     _port->open();
     _port->setTimeout(TIMEOUT/100);
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
      cerr<<"Error opening Port"<<endl;
      return false;
   }

   _port->flushPort();
   fThread = new std::thread( [this] { this->eventLoop(); } );
   fActiveLoop=true;
   return true;
}

void Ublox::Disconnect()
{
   while (_port->isOpen())
      _port->closePort();
   fActiveLoop = false;
   if (fThread) fThread->join();
   delete fThread;
}


int Ublox::ReadBuffer(std::string& buf)
{
//   std::string s="";
//   if (!_port->IsDataAvailable()) return s;
   int n=0;
   //try
   {
      // Wait for Data and read
      std::string s1;
      
      n=_port->read(buf,250);
//      cout<<s1<<endl;
   }
/*
   catch (...)
   {
      cerr<<"Timeout!"<<endl;
   }*/

   return n;
}

int Ublox::WriteBuffer(const std::string& buf)
{
//   std::string s="";
//   if (!_port->IsDataAvailable()) return s;
   int n=0;
   //try
   {
      n=_port->write(buf);
//      cout<<s1<<endl;
   }
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
  *chkA=0;
  *chkB=0;
  for (int i=2; i<buf.size(); i++)
  {
    *chkA += buf[i];
    *chkB += *chkA;
  }
}

bool Ublox::sendUBX(uint16_t msgID, unsigned char* payload, int nBytes)
{
  std::string s="";
  s+=0xb5; s+=0x62;
  s+=(unsigned char)((msgID & 0xff00)>>8);
  s+=(unsigned char)(msgID & 0xff);
  s+=(unsigned char)(nBytes & 0xff);
  s+=(unsigned char)((nBytes & 0xff00)>>8);
  for (int i=0; i<nBytes; i++) s+=payload[i];
  unsigned char chkA, chkB;
  calcChkSum(s, &chkA, &chkB);
  s+=chkA;
  s+=chkB;
//   cout<<endl<<endl;
//   cout<<"write UBX string: ";
//   for (int i=0; i<s.size(); i++) cout<<hex<<(int)s[i]<<" ";
//   cout<<endl<<endl;
  if (s.size() == WriteBuffer(s)) return true;
  return false;
}

bool Ublox::sendUBX(unsigned char classID, unsigned char messageID, unsigned char* payload, int nBytes)
{
  uint16_t msgID = messageID + (uint16_t)classID<<8;
  return sendUBX(msgID,payload,nBytes);
}


bool Ublox::waitAck(int timeout)
{
  int iter=0;
  std::string answer;
  while (iter<timeout && !answer.size())
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
		std::this_thread::sleep_for(std::chrono::milliseconds(1));    
// 		usleep(1000);
		iter++;
		for (std::list<UbxMessage>::iterator it=fMessageBuffer.begin(); it!=fMessageBuffer.end(); it++) {
// 			if (it->classID==0x05 && it->messageID<2) {
			if (it->msgID>=0x0500 && it->msgID<0x0502) {
				answer=it->data;
				fMessageBuffer.erase(it);
				break;
			} 
		}
    }
    if (iter>=timeout) return false;
    if (fVerbose>2) {
      cout<<"received UBX ACK string: ";
      for (int i=0; i<answer.size(); i++) cout<<hex<<(int)answer[i]<<" ";
      cout<<endl;
    }

  return true;
}

bool Ublox::waitAck(uint16_t msgID, int timeout)
{
  int iter=0;
  const static int cycleSleepMs = 10;
  std::string answer;
  int mid=-1;
  while (iter<timeout/cycleSleepMs && !answer.size())
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
		std::this_thread::sleep_for(std::chrono::milliseconds(cycleSleepMs));    
// 		usleep(1000);
		iter++;
		for (std::list<UbxMessage>::iterator it=fMessageBuffer.begin(); it!=fMessageBuffer.end(); it++) {
			if (it->msgID>=0x0500 && it->msgID<0x0502) {
			  if (fVerbose>4) {
			    cout<<"MSG-ACK ok: data[0]="<<(int)it->data[0]<<" data[1]="<<(int)it->data[1]<<" size="<<it->data.size()<<" "<<hex<<msgID<<endl;
			  }
			  if (it->data.size()==2 && it->data[0]==(uint8_t)((msgID & 0xff00)>>8) && it->data[1]==(msgID & 0xff)) {
			    mid=it->msgID;
			    answer=it->data;
			    fMessageBuffer.erase(it);
			    break;
			  }
			} 
		}
    }
    if (iter>=timeout/cycleSleepMs) return false;
    
    if (fVerbose>2) {
      cout<<"received UBX "<<std::string((mid==0x0501)?"ACK":"NAK")<<" message";
      if (fVerbose>3) {
	cout<<": ";
	for (int i=0; i<answer.size(); i++) cout<<hex<<(int)answer[i]<<" ";
      }
      cout<<endl;
    }
    return true;
}

bool Ublox::waitAck(uint8_t classID, uint8_t messageID, int timeout)
{
  uint16_t msgID = messageID + (uint16_t)classID<<8;
  return waitAck(msgID,timeout);
}

bool Ublox::pollUBX(uint16_t msgID, std::string& answer, int timeout)
{
    sendUBX(msgID, NULL, 0);
    int iter=0;
    answer="";
    std::string buffer="";
    while (iter<timeout && !answer.size())
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
		std::this_thread::sleep_for(std::chrono::milliseconds(1));    
		iter++;
		for (std::list<UbxMessage>::iterator it=fMessageBuffer.begin(); it!=fMessageBuffer.end(); it++) {
			if (it->msgID==msgID) {
				answer=it->data;
				fMessageBuffer.erase(it);
				break;
			} 
		}
    }
    if (iter>=timeout) return false;
    if (fVerbose>2) {
      cout<<"received UBX string: ";
      for (int i=0; i<answer.size(); i++) cout<<hex<<(int)answer[i]<<" ";
      cout<<endl;
    }
    if (answer.size()) return true;
    return false;
}

bool Ublox::pollUBX(uint8_t classID, uint8_t messageID, std::string& answer, int timeout)
{
  uint16_t msgID = messageID + (uint16_t)classID<<8;
  return pollUBX(msgID,answer,timeout);
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
//     if (fVerbose>2) {
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
{
    if (buffer.size()<9) return string();
    std::string refstr="";
    refstr+=0xb5; refstr+=0x62;
//    cout<<"hier"<<endl;
    int found=buffer.find(refstr);
    std::string mess="";
    bool result=false;
    if (found!=string::npos)
    {
      mess=buffer.substr(found, buffer.size());
//	    cout<<"da1"<<endl;
      buffer.erase(0,found);
      //result=true;
    } else return string();
    msgID = (uint16_t)mess[2]<<8;
    msgID += (uint16_t)mess[3];
    if (mess.size()<8) return string();
//        cout<<"received UBX string: ";
//        for (int i=0; i<mess.size(); i++) cout<<hex<<(int)mess[i]<<" ";
//        cout<<endl;
    int len=(int)(mess[4]);
    len+=((int)(mess[5]))<<8;
    if (mess.size()-8<len) {
      int found=buffer.find(refstr,2);
      if (found!=string::npos) {
	cout<<"received faulty UBX string: ";
	for (int i=0; i<found; i++) cout<<hex<<(int)buffer[i]<<" ";
	cout<<dec<<endl;
	buffer.erase(0,found);
      }
      return string();
    }
//    cout<<"len="<<dec<<len<<endl;
//    cout<<"mess.size()="<<dec<<mess.size()<<endl;
//	cout<<"da2"<<endl;
    buffer.erase(0,len+8);
    
    unsigned char chkA;
    unsigned char chkB;
    calcChkSum(mess.substr(0,len+6), &chkA, &chkB);
//     cout<<"chkA/B="<<hex<<(int)chkA<<" "<<(int)chkB<<endl;
//     cout<<"mess[len+6/7]="<<hex<<(int)mess[len+6]<<" "<<(int)mess[len+7]<<endl;
    if (mess[len+6]==chkA && mess[len+7]==chkB)
    {
		return mess.substr(6,len);

	}
    
    return string();
}

std::string Ublox::scanUnknownMessage(std::string& buffer, uint8_t& classID, uint8_t& messageID)
{
  uint16_t msgID = messageID + (uint16_t)classID<<8;
  return scanUnknownMessage(buffer, msgID);
}

bool Ublox::UBXNavClock(int& itow, int& bias, int& drift, int& tAccuracy, int& fAccuracy)
{
  std::string answer;
  // UBX-NAV-CLOCK: clock solution
  bool ok=pollUBX(MSG_NAV_CLOCK, answer, MSGTIMEOUT);
  if (!ok) return ok;
  //return true;
  // parse all fields
  // GPS time of week
  uint32_t iTOW = (int)answer[0];
  iTOW+=((int)answer[1])<<8;
  iTOW+=((int)answer[2])<<16;
  iTOW+=((int)answer[3])<<24;
  itow=iTOW/1000;
  // clock bias
  if (fVerbose>3) {
    cout<<"clkB[0]="<<hex<<(int)answer[4]<<endl;
    cout<<"clkB[1]="<<hex<<(int)answer[5]<<endl;
    cout<<"clkB[2]="<<hex<<(int)answer[6]<<endl;
    cout<<"clkB[3]="<<hex<<(int)answer[7]<<endl;
  }
  int32_t clkB=(int)answer[4];
  clkB+=((int)answer[5])<<8;
  clkB+=((int)answer[6])<<16;
  clkB+=((int)answer[7])<<24;
  bias=clkB;
  // clock drift
  int32_t clkD=(int)answer[8];
  clkD+=((int)answer[9])<<8;
  clkD+=((int)answer[10])<<16;
  clkD+=((int)answer[11])<<24;
  drift=clkD;
  // time accuracy estimate
  uint32_t tAcc = (int)answer[12];
  tAcc+=((int)answer[13])<<8;
  tAcc+=((int)answer[14])<<16;
  tAcc+=((int)answer[15])<<24;
//  tAccuracy=tAcc;
  // freq accuracy estimate
  uint32_t fAcc = (int)answer[16];
  fAcc+=((int)answer[17])<<8;
  fAcc+=((int)answer[18])<<16;
  fAcc+=((int)answer[19])<<24;
  fAccuracy=fAcc;
  if (fVerbose>1) {
    cout<<"*** UBX-NAV-CLOCK message:"<<endl;
    cout<<" iTOW          : "<<dec<<iTOW/1000<<" s"<<endl;
    cout<<" clock bias    : "<<dec<<clkB<<" ns"<<endl;
    cout<<" clock drift   : "<<dec<<clkD<<" ns/s"<<endl;
    cout<<" time accuracy : "<<dec<<tAcc<<" ns"<<endl;
    cout<<" freq accuracy : "<<dec<<fAcc<<" ps/s"<<endl;
  }
  return ok;
}

bool Ublox::UBXTimTP(int& itow, int& quantErr, int& weekNr)
{
  std::string answer;
  // UBX-TIM-TP: time pulse timedata
  bool ok=pollUBX(MSG_TIM_TP, answer, MSGTIMEOUT);
  if (!ok) return ok;
  // parse all fields
  // TP time of week, ms
  uint32_t towMS = (int)answer[0];
  towMS+=((int)answer[1])<<8;
  towMS+=((int)answer[2])<<16;
  towMS+=((int)answer[3])<<24;
  // TP time of week, sub ms
  uint32_t towSubMS = (int)answer[4];
  towSubMS+=((int)answer[5])<<8;
  towSubMS+=((int)answer[6])<<16;
  towSubMS+=((int)answer[7])<<24;
  itow=towMS/1000;
  // quantization error
  int32_t qErr=(int)answer[8];
  qErr+=((int)answer[9])<<8;
  qErr+=((int)answer[10])<<16;
  qErr+=((int)answer[11])<<24;
  quantErr=qErr;
  // week number
  uint16_t week=(int)answer[12];
  week+=((int)answer[13])<<8;
  weekNr=week;
  // flags
  uint8_t flags=answer[14];
  // ref info
  uint8_t refInfo=answer[15];

  double sr = towMS/1000.;
  sr=sr-towMS/1000;
  
  if (fVerbose>1) {
    cout<<"*** UBX-TIM-TP message:"<<endl;
    cout<<" tow s            : "<<dec<<towMS/1000.<<" s"<<endl;
    cout<<" tow sub s        : "<<dec<<towSubMS<<" = "<<(long int)(sr*1e9+towSubMS+0.5)<<" ns"<<endl;
    cout<<" quantization err : "<<dec<<qErr<<" ps"<<endl;
    cout<<" week nr          : "<<dec<<week<<endl;
    cout<<" flags            : ";
    for (int i=7; i>=0; i--) if (flags & 1<<i) cout<<i; else cout<<"-";
    cout<<endl;
    cout<<" refInfo          : ";
    for (int i=7; i>=0; i--) if (refInfo & 1<<i) cout<<i; else cout<<"-";
    cout<<endl;
  }
  return ok;
}

bool Ublox::UBXTimTP()
{
  std::string answer;
  // UBX-TIM-TP: time pulse timedata
  bool ok=pollUBX(MSG_TIM_TP, answer, MSGTIMEOUT);
  if (!ok) return ok;
  UBXTimTP(answer);
  return ok;
}

bool Ublox::UBXTimTP(const std::string& msg)
{
  // parse all fields
  // TP time of week, ms
  uint32_t towMS = (int)msg[0];
  towMS+=((int)msg[1])<<8;
  towMS+=((int)msg[2])<<16;
  towMS+=((int)msg[3])<<24;
  // TP time of week, sub ms
  uint32_t towSubMS = (int)msg[4];
  towSubMS+=((int)msg[5])<<8;
  towSubMS+=((int)msg[6])<<16;
  towSubMS+=((int)msg[7])<<24;
  int itow=towMS/1000;
  // quantization error
  int32_t qErr=(int)msg[8];
  qErr+=((int)msg[9])<<8;
  qErr+=((int)msg[10])<<16;
  qErr+=((int)msg[11])<<24;
  int quantErr=qErr;
  TPQuantErr = qErr;
  // week number
  uint16_t week=(int)msg[12];
  week+=((int)msg[13])<<8;
  int weekNr=week;
  // flags
  uint8_t flags=msg[14];
  // ref info
  uint8_t refInfo=msg[15];

  double sr = towMS/1000.;
  sr=sr-towMS/1000;

//   cout<<"0d 01 "<<dec<<weekNr<<" "<<towMS/1000<<" "<<(long int)(sr*1e9+towSubMS+0.5)<<" "<<qErr<<flush<<endl;

  if (fVerbose>1) {
    cout<<"*** UBX-TIM-TP message:"<<endl;
    cout<<" tow s            : "<<dec<<towMS/1000.<<" s"<<endl;
    cout<<" tow sub s        : "<<dec<<towSubMS<<" = "<<(long int)(sr*1e9+towSubMS+0.5)<<" ns"<<endl;
    cout<<" quantization err : "<<dec<<qErr<<" ps"<<endl;
    cout<<" week nr          : "<<dec<<week<<endl;
    cout<<" *flags            : ";
    for (int i=7; i>=0; i--) if (flags & 1<<i) cout<<i; else cout<<"-";
    cout<<endl;
    cout<<"  time base     : "<<string(((flags&1)?"UTC":"GNSS"))<<endl;
    cout<<"  UTC available : "<<string((flags&2)?"yes":"no")<<endl;
    cout<<"  (T)RAIM info  : "<<(int)((flags& 0x0c)>>2)<<endl;
    cout<<" *refInfo          : ";
    for (int i=7; i>=0; i--) if (refInfo & 1<<i) cout<<i; else cout<<"-";
    cout<<endl;
    string gnssRef;
    switch (refInfo&0x0f) {
		case 0:
			gnssRef="GPS";
			break;
		case 1:
			gnssRef="GLONASS";
			break;
		case 2:
			gnssRef="BeiDou";
			break;
		default:
			gnssRef="unknown";
	}
    cout<<"  GNSS reference : "<<gnssRef<<endl;
    string utcStd;
    switch ((refInfo&0xf0)>>4) {
		case 0:
			utcStd="n/a";
			break;
		case 1:
			utcStd="CRL";
			break;
		case 2:
			utcStd="NIST";
			break;
		case 3:
			utcStd="USNO";
			break;
		case 4:
			utcStd="BIPM";
			break;
		case 5:
			utcStd="EU";
			break;
		case 6:
			utcStd="SU";
			break;
		default:
			utcStd="unknown";
	}
    cout<<"  UTC standard  : "<<utcStd<<endl;
	
  }
  return true;
}

bool Ublox::UBXTimTM2(const std::string& msg)
{
  // parse all fields
  // channel
  uint8_t ch=msg[0];
  // flags
  uint8_t flags=msg[1];
  // rising edge counter
  uint16_t count=(int)msg[2];
  count+=((int)msg[3])<<8;
  // week number of last rising edge
  uint16_t wnR=(int)msg[4];
  wnR+=((int)msg[5])<<8;
  // week number of last falling edge
  uint16_t wnF=(int)msg[6];
  wnF+=((int)msg[7])<<8;
  // time of week of rising edge, ms
  uint32_t towMsR = (int)msg[8];
  towMsR+=((int)msg[9])<<8;
  towMsR+=((int)msg[10])<<16;
  towMsR+=((int)msg[11])<<24;
  // time of week of rising edge, sub ms
  uint32_t towSubMsR = (int)msg[12];
  towSubMsR+=((int)msg[13])<<8;
  towSubMsR+=((int)msg[14])<<16;
  towSubMsR+=((int)msg[15])<<24;
  // time of week of falling edge, ms
  uint32_t towMsF = (int)msg[16];
  towMsF+=((int)msg[17])<<8;
  towMsF+=((int)msg[18])<<16;
  towMsF+=((int)msg[19])<<24;
  // time of week of falling edge, sub ms
  uint32_t towSubMsF = (int)msg[20];
  towSubMsF+=((int)msg[21])<<8;
  towSubMsF+=((int)msg[22])<<16;
  towSubMsF+=((int)msg[23])<<24;
  // accuracy estimate
  uint32_t accEst=(int)msg[24];
  accEst+=((int)msg[25])<<8;
  accEst+=((int)msg[26])<<16;
  accEst+=((int)msg[27])<<24;

  timeAccuracy = accEst;
  
  double sr = towMsR/1000.;
  sr=sr-towMsR/1000;
//  sr*=1000.;
  double sf = towMsF/1000.;
  sf=sf-towMsF/1000;
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
  
  if (fVerbose>1) {
  cout<<"*** UBX-TimTM2 message:"<<endl;
  cout<<" channel         : "<<dec<<(int)ch<<endl;
  cout<<" rising edge ctr : "<<dec<<count<<endl;
  cout<<" * last rising edge:"<<endl;
  cout<<"    week nr        : "<<dec<<wnR<<endl;
  cout<<"    tow s          : "<<dec<<towMsR/1000.<<" s"<<endl;
  cout<<"    tow sub s     : "<<dec<<towSubMsR<<" = "<<(long int)(sr*1e9+towSubMsR)<<" ns"<<endl;
  cout<<" * last falling edge:"<<endl;
  cout<<"    week nr        : "<<dec<<wnF<<endl;
  cout<<"    tow s          : "<<dec<<towMsF/1000.<<" s"<<endl;
  cout<<"    tow sub s      : "<<dec<<towSubMsF<<" = "<<(long int)(sf*1e9+towSubMsF)<<" ns"<<endl;
  cout<<" accuracy est      : "<<dec<<accEst<<" ns"<<endl;
  cout<<" flags             : ";
  for (int i=7; i>=0; i--) if (flags & 1<<i) cout<<i; else cout<<"-";
  cout<<endl;
  cout<<"   mode                 : "<<string((flags&1)?"single":"running")<<endl;
  cout<<"   run                  : "<<string((flags&2)?"armed":"stopped")<<endl;
  cout<<"   new rising edge      : "<<string((flags&0x80)?"yes":"no")<<endl;
  cout<<"   new falling edge     : "<<string((flags&0x04)?"yes":"no")<<endl;
  cout<<"   UTC available        : "<<string((flags&0x20)?"yes":"no")<<endl;
  cout<<"   time valid (GNSS fix): "<<string((flags&0x40)?"yes":"no")<<endl;
    string timeBase;
    switch ((flags&0x18)>>3) {
		case 0:
			timeBase="receiver time";
			break;
		case 1:
			timeBase="GNSS";
			break;
		case 2:
			timeBase="UTC";
			break;
		default:
			timeBase="unknown";
	}
  cout<<"   time base            : "<<timeBase<<endl;

  }
  
  struct timespec ts_r = unixtime_from_gps(wnR,towMsR/1000,(long int)(sr*1e9+towSubMsR),this->leapSeconds());
  struct timespec ts_f = unixtime_from_gps(wnF,towMsF/1000,(long int)(sf*1e9+towSubMsF),this->leapSeconds());
  
  struct gpsTimestamp ts;
  ts.rising_time=ts_r;
  ts.falling_time=ts_f;
  ts.valid=(flags&0x40);
  ts.channel=ch;
  
//   fTimestamps.push(ts);
  
  eventCounter=count;
  ts.counter=count;
  ts.accuracy_ns=accEst;
  
  ts.rising=ts.falling=false;
  if (flags&0x80) {
    // new rising edge detected
    ts.rising=true;
    
    
  } if (flags&0x04) {
    // new falling edge detected
    ts.falling=true;
    
  }
    fTimestamps.push(ts);
  
  
  return true;
}

std::vector<GnssSatellite> Ublox::UBXNavSat(bool allSats)
{
  std::string answer;
  std::vector<GnssSatellite> satList;
  // UBX-NAV-SAT: satellite information
  bool ok=pollUBX(MSG_NAV_SAT, answer, MSGTIMEOUT);
//  bool ok=pollUBX(0x0d, 0x03, 0, answer, MSGTIMEOUT);
  if (!ok) return satList;
  
  return UBXNavSat(answer, allSats);
  
  // parse all fields
  // GPS time of week
  uint32_t iTOW = (int)answer[0];
  iTOW+=((int)answer[1])<<8;
  iTOW+=((int)answer[2])<<16;
  iTOW+=((int)answer[3])<<24;
  // version
  uint8_t version=answer[4];
  uint8_t numSvs=answer[5];
  
  int N=(answer.size()-8)/12;

  if (fVerbose>1) {
    cout<<"*** UBX-NAV-SAT message:"<<endl;
    cout<<" iTOW          : "<<dec<<iTOW/1000<<" s"<<endl;
    cout<<" version       : "<<dec<<(int)version<<endl;
    cout<<" Nr of sats    : "<<(int)numSvs<<"  (nr of sections="<<N<<")"<<endl;
    cout<<"   Sat Data :"<<endl;
  }
  int goodSats=0;
  for (int i=0; i<N; i++) {
    GnssSatellite sat(answer.substr(8+12*i,12));
    if (sat.getCnr()>0) goodSats++;
    satList.push_back(sat);
  }
  if (!allSats) {
    sort(satList.begin(), satList.end(), GnssSatellite::sortByCnr);
    while (satList.back().getCnr()==0 && satList.size()>0) satList.pop_back();
  }
  
  if (fVerbose) {
    GnssSatellite::PrintHeader(true);
    for (vector<GnssSatellite>::iterator it=satList.begin(); it!=satList.end(); it++) {
      it->Print(distance(satList.begin(), it), false);
    }
    cout<<"   --------------------------------------------------------------------"<<endl;
    if (fVerbose>1) cout<<" Nr of avail sats : "<<goodSats<<endl;
  }
  return satList;
}

std::vector<GnssSatellite> Ublox::UBXNavSat(const std::string& msg, bool allSats)
{
  std::vector<GnssSatellite> satList;
  // UBX-NAV-SAT: satellite information
  // parse all fields
  // GPS time of week
  uint32_t iTOW = (int)msg[0];
  iTOW+=((int)msg[1])<<8;
  iTOW+=((int)msg[2])<<16;
  iTOW+=((int)msg[3])<<24;
  // version
  uint8_t version=msg[4];
  uint8_t numSvs=msg[5];
  
  int N=(msg.size()-8)/12;

  if (fVerbose>1) {
    cout<<setfill(' ')<<setw(3);
    cout<<"*** UBX-NAV-SAT message:"<<endl;
    cout<<" iTOW          : "<<dec<<iTOW/1000<<" s"<<endl;
    cout<<" version       : "<<dec<<(int)version<<endl;
    cout<<" Nr of sats    : "<<(int)numSvs<<"  (nr of sections="<<N<<")"<<endl;
    cout<<"   Sat Data :"<<endl;
  }
  int goodSats=0;
  for (int i=0; i<N; i++) {
    GnssSatellite sat(msg.substr(8+12*i,12));
    if (sat.getCnr()>0) goodSats++;
    satList.push_back(sat);
  }
  if (!allSats) {
    sort(satList.begin(), satList.end(), GnssSatellite::sortByCnr);
    while (satList.back().getCnr()==0 && satList.size()>0) satList.pop_back();
  }
  
//  nrSats = satList.size();
  nrSats = goodSats;
  
  if (fVerbose) {
    GnssSatellite::PrintHeader(true);
    for (vector<GnssSatellite>::iterator it=satList.begin(); it!=satList.end(); it++) {
      it->Print(distance(satList.begin(), it), false);
    }
    cout<<"   --------------------------------------------------------------------"<<endl;
    if (fVerbose>1) cout<<" Nr of avail sats : "<<goodSats<<endl;
  }
  return satList;
}


bool Ublox::UBXCfgGNSS()
{
  std::string answer;
  // UBX-CFG-GNSS: GNSS configuration
  bool ok=pollUBX(MSG_CFG_GNSS, answer, MSGTIMEOUT);
  if (!ok) return false;
  // parse all fields
  // version
  uint8_t version=answer[0];
  uint8_t numTrkChHw=answer[1];
  uint8_t numTrkChUse=answer[2];
  uint8_t numConfigBlocks=answer[3];
  
  int N=(answer.size()-4)/8;

//  if (fVerbose>1) 
  {
    cout<<"*** UBX CFG-GNSS message:"<<endl;
    cout<<" version                    : "<<dec<<(int)version<<endl;
    cout<<" nr of hw tracking channels : "<<dec<<(int)numTrkChHw<<endl;
    cout<<" nr of channels in use      : "<<dec<<(int)numTrkChUse<<endl;
    cout<<" Nr of config blocks        : "<<(int)numConfigBlocks<<"  (nr of sections="<<N<<")"<<endl;
    cout<<"  Config Data :"<<endl;
  }
  for (int i=0; i<N; i++) {
    uint8_t gnssID = answer[4+8*i];
    uint8_t resTrkCh = answer[5+8*i];
    uint8_t maxTrkCh = answer[6+8*i];
    uint32_t flags = answer[8+8*i];
    flags|=(int)answer[9+8*i]<<8;
    flags|=(int)answer[10+8*i]<<16;
    flags|=(int)answer[11+8*i]<<24;
//    if (fVerbose>1) 
    {
      cout<<"   "<<i<<":   GNSS name : "<< GnssSatellite::GNSS_ID_STRING[gnssID]<<endl;
      cout<<"      reserved (min) tracking channels  : "<<dec<<(int)resTrkCh<<endl;
      cout<<"      max nr of tracking channels used : "<<dec<<(int)maxTrkCh<<endl;
      cout<<"      flags  : "<<hex<<(int)flags<<endl;
    }
  }
  return true;
}

bool Ublox::UBXCfgNav5()
{
  std::string answer;
  // UBX CFG-NAV5: satellite information
  bool ok=pollUBX(MSG_CFG_NAV5, answer, MSGTIMEOUT);
  if (!ok) return false;
  // parse all fields
  // version
  uint16_t mask=answer[0];
  mask|=(int)answer[1]<<8;
  uint8_t dynModel=answer[2];
  uint8_t fixMode=answer[3];
  int32_t fixedAlt=answer[4];
  fixedAlt|=(int)answer[5]<<8;
  fixedAlt|=(int)answer[6]<<16;
  fixedAlt|=(int)answer[7]<<24;
  uint32_t fixedAltVar=answer[8];
  fixedAltVar|=(int)answer[9]<<8;
  fixedAltVar|=(int)answer[10]<<16;
  fixedAltVar|=(int)answer[11]<<24;
  int8_t minElev=answer[12];
  

//  if (fVerbose>1)
  {
    cout<<"*** UBX CFG-NAV5 message:"<<endl;
    cout<<" mask               : "<<hex<<(int)mask<<endl;
    cout<<" dynamic model used : "<<dec<<(int)dynModel<<endl;
    cout<<" fixMode            : "<<dec<<(int)fixMode<<endl;
    cout<<" fixed Alt          : "<<(double)fixedAlt*0.01<<" m"<<endl;
    cout<<" fixed Alt Var      : "<<(double)fixedAltVar*0.0001<<" m^2"<<endl;
    cout<<" min elevation      : "<<dec<<(int)minElev<<" deg"<<endl;
  }
  return true;
}

std::vector<std::string> Ublox::UBXMonVer()
{
  std::string answer;
  std::vector<std::string> result;
  // UBX CFG-NAV5: satellite information
  bool ok=pollUBX(MSG_MON_VER, answer, MSGTIMEOUT);
  if (!ok) return result;
  // parse all fields
  // version
  cout<<"*** UBX MON-VER message:"<<endl;
//  cout<<answer<<endl;
  int i=0;
  while (i!=std::string::npos && i<answer.size()) {
    std::string s=answer.substr(i,answer.find((char)0x00,i+1)-i+1);
    while (s.size() && s[0]==0x00) {
      s.erase(0,1);
    }
    if (s.size()){
      result.push_back(s);
      cout<<s<<endl;
    }
    i=answer.find((char)0x00,i+1);
  }
  return result;
}

      
void Ublox::Print()
{
   std::cout<<"U-Blox-Device:\n";
   //std::cout<<setfill('0');
   std::cout<<" serial port    : "<<_portName<<std::endl;
}


void Ublox::UBXNavClock(const std::string& msg)
{
  // parse all fields
  // GPS time of week
  uint32_t iTOW = (int)msg[0];
  iTOW+=((int)msg[1])<<8;
  iTOW+=((int)msg[2])<<16;
  iTOW+=((int)msg[3])<<24;
  uint32_t itow=iTOW/1000;
  // clock bias
  if (fVerbose>3) {
    cout<<"clkB[0]="<<hex<<(int)msg[4]<<endl;
    cout<<"clkB[1]="<<hex<<(int)msg[5]<<endl;
    cout<<"clkB[2]="<<hex<<(int)msg[6]<<endl;
    cout<<"clkB[3]="<<hex<<(int)msg[7]<<endl;
  }
  int32_t clkB=(int)msg[4];
  clkB+=((int)msg[5])<<8;
  clkB+=((int)msg[6])<<16;
  clkB+=((int)msg[7])<<24;
  int32_t bias=clkB;
  clkBias=clkB;
  // clock drift
  int32_t clkD=(int)msg[8];
  clkD+=((int)msg[9])<<8;
  clkD+=((int)msg[10])<<16;
  clkD+=((int)msg[11])<<24;
  int32_t drift=clkD;
  clkDrift = clkD;
  // time accuracy estimate
  uint32_t tAcc = (int)msg[12];
  tAcc+=((int)msg[13])<<8;
  tAcc+=((int)msg[14])<<16;
  tAcc+=((int)msg[15])<<24;
  uint32_t tAccuracy=tAcc;
  //timeAccuracy = tAcc;
  // freq accuracy estimate
  uint32_t fAcc = (int)msg[16];
  fAcc+=((int)msg[17])<<8;
  fAcc+=((int)msg[18])<<16;
  fAcc+=((int)msg[19])<<24;
  uint32_t fAccuracy=fAcc;
  
  // meaning of columns:
  // 01 22 - signature of NAV-CLOCK message
  // second in current week (s), clock bias (ns), clock drift (ns/s), time accuracy (ns), freq accuracy (ps/s)
//   cout<<"01 22 "<<dec<<iTOW/1000<<" "<<clkB<<" "<<clkD<<" "<<tAcc<<" "<<fAcc<<flush;
//   cout<<endl;

  
  if (fVerbose>1) {
    cout<<"*** UBX-NAV-CLOCK message:"<<endl;
    cout<<" iTOW          : "<<dec<<iTOW/1000<<" s"<<endl;
    cout<<" clock bias    : "<<dec<<clkB<<" ns"<<endl;
    cout<<" clock drift   : "<<dec<<clkD<<" ns/s"<<endl;
    cout<<" time accuracy : "<<dec<<tAcc<<" ns"<<endl;
    cout<<" freq accuracy : "<<dec<<fAcc<<" ps/s"<<endl;
  }
}


void Ublox::UBXNavTimeGPS(const std::string& msg)
{
  // parse all fields
  // GPS time of week
  uint32_t iTOW = (int)msg[0];
  iTOW+=((int)msg[1])<<8;
  iTOW+=((int)msg[2])<<16;
  iTOW+=((int)msg[3])<<24;

  int32_t fTOW = (int)msg[4];
  fTOW+=((int)msg[5])<<8;
  fTOW+=((int)msg[6])<<16;
  fTOW+=((int)msg[7])<<24;

//  int32_t ftow=iTOW/1000;

  uint16_t wnR=(int)msg[8];
  wnR+=((int)msg[9])<<8;

  int8_t leapS = (int)msg[10];
  uint8_t flags = (int)msg[11];

  // time accuracy estimate
  uint32_t tAcc = (int)msg[12];
  tAcc+=((int)msg[13])<<8;
  tAcc+=((int)msg[14])<<16;
  tAcc+=((int)msg[15])<<24;
  uint32_t tAccuracy=tAcc;
  timeAccuracy = tAcc;

  double sr = iTOW/1000.;
  sr=sr-iTOW/1000;

  // meaning of columns:
  // 01 20 - signature of NAV-TIMEGPS message
  // week nr, second in current week, ns of timestamp in current second,
  // nr of leap seconds wrt UTC, accuracy (ns)
//   cout<<"01 20 "<<dec<<wnR<<" "<<iTOW/1000<<" "<<(long int)(sr*1e9+fTOW)<<" "<<(int)leapS<<" "<<tAcc<<flush;
//   cout<<endl;

  if (fVerbose>1) {
    cout<<"*** UBX-NAV-TIMEGPS message:"<<endl;
    cout<<" week nr       : "<<dec<<wnR<<endl;
    cout<<" iTOW          : "<<dec<<iTOW<<" ms = "<<iTOW/1000<<" s"<<endl;
    cout<<" fTOW          : "<<dec<<fTOW<<" = "<<(long int)(sr*1e9+fTOW)<<" ns"<<endl;
    cout<<" leap seconds  : "<<dec<<(int)leapS<<" s"<<endl;
    cout<<" time accuracy : "<<dec<<tAcc<<" ns"<<endl;
	cout<<" flags             : ";
	for (int i=7; i>=0; i--) if (flags & 1<<i) cout<<i; else cout<<"-";
	cout<<endl;
	cout<<"   tow valid        : "<<string((flags&1)?"yes":"no")<<endl;
	cout<<"   week valid       : "<<string((flags&2)?"yes":"no")<<endl;
	cout<<"   leap sec valid   : "<<string((flags&4)?"yes":"no")<<endl;
  }
  
  if (flags&4) leapSeconds = leapS;
  
  struct timespec ts = unixtime_from_gps(wnR,iTOW/1000,(long int)(sr*1e9+fTOW),this->leapSeconds());
}

void Ublox::UBXNavTimeUTC(const std::string& msg)
{
  // parse all fields
  // GPS time of week
  uint32_t iTOW = (int)msg[0];
  iTOW+=((int)msg[1])<<8;
  iTOW+=((int)msg[2])<<16;
  iTOW+=((int)msg[3])<<24;

  // time accuracy estimate
  uint32_t tAcc = (int)msg[4];
  tAcc+=((int)msg[5])<<8;
  tAcc+=((int)msg[6])<<16;
  tAcc+=((int)msg[7])<<24;
  uint32_t tAccuracy=tAcc;
  timeAccuracy = tAcc;

  int32_t nano = (int)msg[8];
  nano+=((int)msg[9])<<8;
  nano+=((int)msg[10])<<16;
  nano+=((int)msg[11])<<24;

  uint16_t year=(int)msg[12];
  year+=((int)msg[13])<<8;

  uint16_t month=(int)msg[14];
  uint16_t day=(int)msg[15];
  uint16_t hour=(int)msg[16];
  uint16_t min=(int)msg[17];
  uint16_t sec=(int)msg[18];

  uint8_t flags = (int)msg[19];

  double sr = iTOW/1000.;
  sr=sr-iTOW/1000;

  // meaning of columns:
  // 01 21 - signature of NAV-TIMEUTC message
  // second in current week, year, month, day, hour, minute, seconds(+fraction)
  // accuracy (ns)
//   cout<<"01 21 "<<dec<<iTOW/1000<<" "<<year<<" "<<month<<" "<<day<<" "<<hour<<" "<<min<<" "<<(double)(sec+nano*1e-9)<<" "<<tAcc<<flush;
//   cout<<endl;

  if (fVerbose>1) {
    cout<<"*** UBX-NAV-TIMEUTC message:"<<endl;
    cout<<" iTOW           : "<<dec<<iTOW<<" ms = "<<iTOW/1000<<" s"<<endl;
    cout<<" nano           : "<<dec<<nano<<" ns"<<endl;
    cout<<" date y/m/d     : "<<dec<<(int)year<<"/"<<(int)month<<"/"<<(int)day<<endl;
    cout<<" UTC time h:m:s : "<<dec<<setw(2)<<setfill('0')<<hour<<":"<<(int)min<<":"<<(double)(sec+nano*1e-9)<<endl;
    cout<<" time accuracy : "<<dec<<tAcc<<" ns"<<endl;
	cout<<" flags             : ";
	for (int i=7; i>=0; i--) if (flags & 1<<i) cout<<i; else cout<<"-";
	cout<<endl;
	cout<<"   tow valid        : "<<string((flags&1)?"yes":"no")<<endl;
	cout<<"   week valid       : "<<string((flags&2)?"yes":"no")<<endl;
	cout<<"   UTC time valid   : "<<string((flags&4)?"yes":"no")<<endl;
    string utcStd;
    switch ((flags&0xf0)>>4) {
		case 0:
			utcStd="n/a";
			break;
		case 1:
			utcStd="CRL";
			break;
		case 2:
			utcStd="NIST";
			break;
		case 3:
			utcStd="USNO";
			break;
		case 4:
			utcStd="BIPM";
			break;
		case 5:
			utcStd="EU";
			break;
		case 6:
			utcStd="SU";
			break;
		case 7:
			utcStd="NTSC";
			break;
		default:
			utcStd="unknown";
	}
    cout<<"   UTC standard  : "<<utcStd<<endl;
  }
}

void Ublox::UBXMonHW(const std::string& msg)
{
  // parse all fields
  // noise
  uint16_t noisePerMS = (int)msg[16];
  noisePerMS+=((int)msg[17])<<8;
  noise = noisePerMS;
  
  // agc
  uint16_t agcCnt = (int)msg[18];
  agcCnt+=((int)msg[19])<<8;
  agc = agcCnt;
  
  uint8_t flags = (int)msg[22];

  // meaning of columns:
  // 01 21 - signature of NAV-TIMEUTC message
  // second in current week, year, month, day, hour, minute, seconds(+fraction)
  // accuracy (ns)
//   cout<<"0a 09 "<<dec<<noisePerMS<<" "<<agcCnt<<" "<<flush;
//   cout<<endl;

  if (fVerbose>1) {
    cout<<"*** UBX-MON-HW message:"<<endl;
    cout<<" noise            : "<<dec<<noisePerMS<<" dBc"<<endl;
    cout<<" agcCnt (0..8192) : "<<dec<<agcCnt<<endl;
	cout<<" flags             : ";
	for (int i=7; i>=0; i--) if (flags & 1<<i) cout<<i; else cout<<"-";
	cout<<endl;
	cout<<"   RTC calibrated   : "<<string((flags&1)?"yes":"no")<<endl;
	cout<<"   safe boot        : "<<string((flags&2)?"yes":"no")<<endl;
	cout<<"   jamming state    : "<<(int)((flags&0x0c)>>2)<<endl;
	cout<<"   Xtal absent      : "<<string((flags&0x10)?"yes":"no")<<endl;
  }
}

void Ublox::UBXMonTx(const std::string& msg)
{
  // parse all fields
  // nr bytes pending
  uint16_t pending[6];
  uint8_t usage[6];
  uint8_t peakUsage[6];
  uint8_t tUsage;
  uint8_t tPeakUsage;
  uint8_t errors;
  
  for (int i=0; i<6; i++) {
    pending[i]=msg[2*i];
    pending[i]|=(uint16_t)msg[2*i+1]<<8;
    usage[i]=msg[i+12];
    peakUsage[i]=msg[i+18];
  }
  
  tUsage = msg[24];
  tPeakUsage = msg[25];
  errors = msg[26];
  
  // meaning of columns:
  // 01 21 - signature of NAV-TIMEUTC message
  // second in current week, year, month, day, hour, minute, seconds(+fraction)
  // accuracy (ns)
//   cout<<"0a 09 "<<dec<<noisePerMS<<" "<<agcCnt<<" "<<flush;
//   cout<<endl;
  txBufUsage = tUsage;
  txBufPeakUsage = tPeakUsage;
  
  if (fVerbose>1) {
    cout<<setfill(' ')<<setw(3);
    cout<<"*** UBX-MON-TXBUF message:"<<endl;
    cout<<" global TX buf usage      : "<<dec<<(int)tUsage<<" %"<<endl;
    cout<<" global TX buf peak usage : "<<dec<<(int)tPeakUsage<<" %"<<endl;
    cout<<" TX buf usage for target      : ";
    for (int i=0; i<6; i++) {
      cout<<"    ("<<dec<<i<<") "<<setw(3)<<(int)usage[i];
    }
    cout<<endl;
    cout<<" TX buf peak usage for target : ";
    for (int i=0; i<6; i++) {
      cout<<"    ("<<dec<<i<<") "<<setw(3)<<(int)peakUsage[i];
    }
    cout<<endl;
    cout<<" TX bytes pending for target  : ";
    for (int i=0; i<6; i++) {
      cout<<"    ("<<dec<<i<<") "<<setw(3)<<pending[i];
    }
    cout<<setw(1)<<setfill(' ')<<endl;
    
  }
}

bool Ublox::UBXSetCfgMsg(uint16_t msgID, uint8_t port, uint8_t rate)
{
  unsigned char data[8]; 
  if (port>5) return false;
  data[0]=(uint8_t)((msgID & 0xff00)>>8);
  data[1]=msgID & 0xff;
//   cout<<"data[0]="<<(int)data[0]<<" data[1]="<<(int)data[1]<<endl;
  data[2+port]=rate;
  sendUBX(MSG_CFG_MSG, data, 8);
  if (waitAck(MSG_CFG_MSG, 12000)) cout<<"Set CFG successful"<<endl;
  else cout<<"Set CFG timeout"<<endl;
  return true;
}

bool Ublox::UBXSetCfgMsg(uint8_t classID, uint8_t messageID, uint8_t port, uint8_t rate)
{
  uint16_t msgID = messageID + (uint16_t)classID<<8;
  return UBXSetCfgMsg(msgID,port,rate);
}

bool Ublox::UBXSetCfgRate(uint8_t measRate, uint8_t navRate)
{
  unsigned char data[6]; 
  if (measRate<10 || navRate<1 || navRate>127) return false;
  data[0]=measRate & 0xff;
  data[1]=(measRate & 0xff00)>>8;
  data[2]=navRate & 0xff;
  data[3]=(navRate & 0xff00)>>8;
  data[4]=0;
  data[5]=0;
  
  sendUBX(MSG_CFG_RATE, data, sizeof(data));
  if (waitAck(MSG_CFG_RATE, 10000)) cout<<"Set CFG successful"<<endl;
  else cout<<"Set CFG timeout"<<endl;
  return true;
}

