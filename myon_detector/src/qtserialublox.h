#ifndef QTSERIALUBLOX_H
#define QTSERIALUBLOX_H

#include <ublox_structs.h>
#include <queue>
#include <string>
#include <QSerialPort>
#include <QObject>
#include <QTimer>

class QtSerialUblox : public QObject
{
	Q_OBJECT

public:
	explicit QtSerialUblox(const QString serialPortName, int newTimeout, int baudRate,
		bool newDumpRaw, int newVerbose, bool newShowout, bool newShowin, QObject *parent = 0);

signals:
	// all messages coming from QtSerialUblox class that should be displayed on console
	// get sent to Client thread with signal/slot mechanics
	void toConsole(QString data);
	void UBXReceivedAckNak(uint16_t ackedMsgID, uint16_t ackedCfgMsgID);
	// ackedMsgID contains the return value of the ack msg (in case of CFG_MSG that is CFG_MSG)
	void UBXreceivedMsgRateCfg(uint16_t msgID, uint8_t rate);
	void UBXCfgError(QString data);
	void gpsRestart();
	void gpsConnectionError();
	// information about updated properties
	void gpsTimeTM2(uint16_t rising, uint16_t falling,
		uint8_t accEst, bool valid, uint8_t timeBase, bool utc);
	void gpsPropertyUpdatedUint8(uint8_t data,
		std::chrono::duration<double> updateAge,
		char propertyName);
	void gpsPropertyUpdatedUint32(uint32_t data,
		std::chrono::duration<double> updateAge,
		char propertyName);
	void gpsPropertyUpdatedInt32(int32_t data,
		std::chrono::duration<double> updateAge,
		char propertyName);
	void gpsPropertyUpdatedGnss(std::vector<GnssSatellite>,
		std::chrono::duration<double> updateAge);

public slots:
	// all functions that can be called from other classes through signal/slot mechanics
	void makeConnection();
	void onReadyRead();
    void onRequestGpsProperties();
	void pollMsgRate(uint16_t msgID);
	void pollMsg(uint16_t msgID);
	// for polling the port configuration for specific port set rate to port ID
	void handleError(QSerialPort::SerialPortError serialPortError);
	void UBXSetCfgMsgRate(uint16_t msgID, uint8_t port, uint8_t rate);
	void UBXSetCfgRate(uint8_t measRate, uint8_t navRate);
	void UBXSetCfgPrt(uint8_t port, uint8_t outProtocolMask);
	void ackTimeout();
	// outPortMask is something like 1 for only UBX protocol or 0b11 for UBX and NMEA


private:
	// all functions for sending and receiving raw data used by other functions in "public slots" section
	// and scanning raw data up to the point where "UbxMessage" object is generated
	bool scanUnknownMessage(std::string &buffer, UbxMessage &message);
	void calcChkSum(const std::string &buf, unsigned char* chkA, unsigned char* chkB);
	bool sendUBX(uint16_t msgID, std::string& payload, uint16_t nBytes);
	bool sendUBX(uint16_t msgID, unsigned char* payload, uint16_t nBytes);
	bool sendUBX(UbxMessage &msg);
	void sendQueuedMsg(bool afterTimeout = false);
	void delay(int millisecondsWait);

	// all functions only used for processing and showing "UbxMessage"
	void processMessage(const UbxMessage& msg);
	bool UBXNavClock(uint32_t& itow, int32_t& bias, int32_t& drift,
		uint32_t& tAccuracy, uint32_t& fAccuracy);
	bool UBXTimTP(uint32_t& itow, int32_t& quantErr, uint16_t& weekNr);
	bool UBXTimTP();
	bool UBXTimTP(const std::string& msg);
	bool UBXTimTM2(const std::string& msg);
	std::vector<GnssSatellite> UBXNavSat(bool allSats);
	std::vector<GnssSatellite> UBXNavSat(const std::string& msg, bool allSats);
	bool UBXCfgGNSS();
	bool UBXCfgNav5();
	std::vector<std::string> UBXMonVer();
	void UBXNavClock(const std::string& msg);
	void UBXNavTimeGPS(const std::string& msg);
	void UBXNavTimeUTC(const std::string& msg);
	void UBXMonHW(const std::string& msg);
	void UBXMonTx(const std::string& msg);


	// all global variables used in QtSerialUblox class until UbxMessage was created
	QSerialPort *serialPort = nullptr;
	QString _portName;
	std::string buffer = "";
	int _baudRate = 0;
	int verbose = 0;
	int timeout = 5000;
	bool dumpRaw = false; // if true show all messages coming from the gps board that can
						  // be interpreted as QString by QString(message) (basically all NMEA)
	bool discardAllNMEA = true; // if true discard all NMEA messages and do not parse them
	bool showout = false; // if true show the ubx messages sent to the gps board as hex
	bool showin = false;
	std::queue <UbxMessage> outMsgBuffer;
	UbxMessage *msgWaitingForAck = 0;
	QTimer *ackTimer;

	// all global variables used for keeping track of satellites and statistics (gpsProperty)
	gpsProperty<int> leapSeconds;
	gpsProperty<double> noise;
	gpsProperty<double> agc;
	gpsProperty<uint8_t> nrSats;
	gpsProperty<uint8_t> TPQuantErr;
	gpsProperty<uint8_t> txBufUsage;
	gpsProperty<uint8_t> txBufPeakUsage;
	gpsProperty<uint32_t> timeAccuracy;
	gpsProperty<uint32_t> eventCounter;
	gpsProperty<int32_t> clkBias;
	gpsProperty<int32_t> clkDrift;
	gpsProperty<std::vector<GnssSatellite> > satList;
	const int	MSGTIMEOUT = 1500;
	std::queue<gpsTimestamp> fTimestamps;
	//std::list<UbxMessage> fMessageBuffer;
};

#endif // QTSERIALUBLOX_H
