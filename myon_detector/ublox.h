#ifndef _UBLOX_H
#define _UBLOX_H


#include <string>
#include <vector>
//#include <sstream>
#include <utility>
#include <inttypes.h>  // uint8_t, etc
#include <string>
//#include <thread>
#include <queue>
#include <list>
#include <mutex>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include "gnsssatellite.h"
#include "structs_and_defines.h"

//#include <serial.h>
#include "serial.h"
#define DEFAULT_VERBOSITY 0
#define deg "\u00B0"
#define MAX_MESSAGE_BUFSIZE 1000

//class SerialPort;
class Serial;

class Ublox : public QObject//QThread
{
    Q_OBJECT

public:
    Ublox(const std::string& serialPortName, int baudRate, QObject *parent = 0, bool dumpRaw = 0, int verbose = DEFAULT_VERBOSITY);
	~Ublox();

    void loop();//run() override;

	//void setDevice(const std::string& serialPortName);
	//void setBaudRate(int);

	void Print();
	inline void setVerbosity(int verbosity) { fVerbose = verbosity; }
	inline int getVerbosity() const { return fVerbose; }

    unsigned int ReadBuffer(std::string& buf);
    unsigned int WriteBuffer(const std::string& buf);

	bool sendUBX(unsigned char classID, unsigned char messageID, unsigned char* payload, int nBytes);
	bool sendUBX(uint16_t msgID, unsigned char* payload, int nBytes);
    bool pollUBX(uint8_t classID, uint8_t messageID, std::string& answer, int timeout, int verbose);
    bool pollUBX(uint16_t msgID, std::string& answer, int timeout, int verbose);
	//       bool pollUBX(uint8_t classID, uint8_t messageID, uint8_t specifier, std::string& answer, int timeout);
	//       bool pollUBX(uint16_t msgID, uint8_t specifier, std::string& answer, int timeout);
	//      std::string scanMessage(const std::string& buffer, uint8_t classID, uint8_t messageID);
	std::string scanUnknownMessage(std::string& buffer, uint8_t& classID, uint8_t& messageID);
    std::string scanUnknownMessage(std::string& buffer, uint16_t& msgID);
    bool waitAck(int timeout, int verbose);
    bool waitAck(uint8_t classID, uint8_t messageID, int timeout, int verbose);
    bool waitAck(uint16_t msgID, int timeout, int verbose);

	//       bool UBXNavClock();
    bool UBXNavClock(uint32_t& itow, int32_t& bias, int32_t& drift,
                     uint32_t& tAccuracy, uint32_t& fAccuracy, int verbose);
    bool UBXTimTP(uint32_t &itow, int32_t &quantErr, uint16_t &weekNr, int verbose);
    bool UBXTimTP(int verbose);
    bool UBXCfgGNSS(int verbose);
    bool UBXCfgNav5(int verbose);

	void pollTimTP();

    std::vector<std::string> UBXMonVer(int verbose);

    gpsTimestamp getEventFIFOEntry();
	unsigned int getEventFIFOSize() const { return fTimestamps.size(); }



    std::vector<GnssSatellite> UBXNavSat(bool allSats = false, int verbose = 0);
    std::vector<GnssSatellite> UBXNavSat(const std::string& msg, bool allSats, int verbose);

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


signals:
    void UBXCfgError(QString error);
    void toConsole(QString message);
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
    void Connect();
    void Disconnect();
    void UBXSetCfgMsg(uint8_t classID, uint8_t messageID, uint8_t port,
                      uint8_t rate, int verbose = DEFAULT_VERBOSITY);
    void UBXSetCfgMsg(uint16_t msgID, uint8_t port,
                      uint8_t rate, int verbose = DEFAULT_VERBOSITY);
    void UBXSetCfgRate(uint8_t measRate, uint8_t navRate,
                       int verbose = DEFAULT_VERBOSITY);


private:
    void processMessage(const UbxMessage& msg, int verbose);
	void calcChkSum(const std::string& buf, unsigned char* chkA, unsigned char* chkB);
    void UBXNavClock(const std::string& msg, int verbose);
    bool UBXTimTM2(const std::string& msg, int verbose);
    bool UBXTimTP(const std::string& msg, int verbose);
    void UBXNavTimeGPS(const std::string& msg, int verbose);
    void UBXNavTimeUTC(const std::string& msg, int verbose);
    void UBXMonHW(const std::string& msg, int verbose);
    void UBXMonTx(const std::string& msg, int verbose);
	void delay(int millisecondsWait);

    QMutex mutex;
	Serial *_port;
	std::string _portName;
	int _baudRate;
	int fVerbose;
	bool quit;
	std::list<UbxMessage> fMessageBuffer;
	bool fDumpRaw;

	std::queue<gpsTimestamp> fTimestamps;
    std::queue<gpsProperty<double >> fQuantErrors;

    std::mutex fTimestampsMutex;   
};

#endif // _UBLOX_H
