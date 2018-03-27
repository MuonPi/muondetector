#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include "tcpconnection.h"
#include "custom_io_operators.h"
#include "ublox.h"

class Client : public QObject
{
	Q_OBJECT

public:
    Client(std::string new_gpsdevname, int new_verbose, bool new_allSats,
        bool new_listSats, bool new_dumpRaw, int new_baudrate, bool new_poll,
        bool new_configGnss, int new_timingCmd, long int new_N, QString serverAddress, quint16 serverPort, QObject *parent = 0);
	void configGps();
	void loop();

public slots:
    //void onPosixTerminateReceived();
    void connectToServer();
    void displaySocketError(int socketError, QString message);
	void displayError(QString message);
    void toConsole(QString data);
    void stoppedConnection(QString hostName, quint16 port, quint32 connectionTimeout, quint32 connectionDuration);
    void gpsPropertyUpdatedInt32(int32_t data, std::chrono::duration<double> updateAge,
                            char propertyName);
    void gpsPropertyUpdatedUint32(uint32_t data, std::chrono::duration<double> updateAge,
                            char propertyName);
    void gpsPropertyUpdatedUint8(uint8_t data, std::chrono::duration<double> updateAge,
                            char propertyName);
    void gpsPropertyUpdatedGnss(std::vector<GnssSatellite>,
                            std::chrono::duration<double> updateAge);

signals:
    //void posixTerminate();
    void sendFile(QString fileName);
    void sendMsg(QString msg);
	void UBXSetCfgMsg(uint8_t classID, uint8_t messageID, uint8_t port, uint8_t rate);
	void UBXSetCfgMsg(uint16_t msgID, uint8_t port, uint8_t rate);
	void UBXSetCfgRate(uint8_t measRate, uint8_t navRate);

private:
    TcpConnection * tcpConnection;
	Ublox *gps;
	QString ipAddress;
	quint16 port;
	void printTimestamp();
	void delay(int millisecondsWait);
	std::string gpsdevname;
	int verbose, timingCmd, baudrate;
	long int N;
	bool allSats, listSats, dumpRaw, poll, configGnss;
};

#endif // CLIENT_H
