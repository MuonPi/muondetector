#ifndef QTSERIALUBLOX_H
#define QTSERIALUBLOX_H

#include "structs_and_defines.h"
#include <queue>
#include <string>
#include <QSerialPort>
#include <QObject>

class QtSerialUblox : public QObject
{
    Q_OBJECT

public:
    explicit QtSerialUblox(const QString serialPortName, int baudRate,
                           bool newDumpRaw, int newVerbose, QObject *parent = 0);

signals:
    // all messages coming from QtSerialUblox class that should be displayed on console
    // get sent to Client thread with signal/slot mechanics
    void toConsole(QString data);
    void UBXCfgError(QString data);

    // information about updated properties
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
    void handleError(QSerialPort::SerialPortError serialPortError);
    void UBXSetCfgMsg(uint16_t msgID, uint8_t port, uint8_t rate);
    void UBXSetCfgRate(uint8_t measRate, uint8_t navRate);


private:
    // all functions for sending and receiving raw data used by other functions in "public slots" section
    // and scanning raw data up to the point where "UbxMessage" object is generated
    void UBXSetCfgMsg2(uint8_t classID, uint8_t messageID, uint8_t port, uint8_t rate);
    bool scanUnknownMessage(std::string &buffer,UbxMessage &message);
    void calcChkSum(const std::string &buf, unsigned char* chkA, unsigned char* chkB);
    bool sendUBX(uint16_t msgID, unsigned char* payload, int nBytes);
    bool sendUBX(unsigned char classID, unsigned char messageID,
                                unsigned char* payload, int nBytes);
    bool pollUBX(uint16_t msgID, std::string& answer, int timeout);
    bool pollUBX(uint8_t classID, uint8_t messageID, std::string& answer, int timeout);


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
    bool dumpRaw = false;
    bool discardAllNMEA = true;

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
    std::list<UbxMessage> fMessageBuffer;

};

#endif // QTSERIALUBLOX_H
