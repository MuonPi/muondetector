#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include <muondetector_shared_global.h>
#include <QTcpSocket>
#include <tcpmessage.h>
#include <time.h>
#include <QTimer>
#include <QFile>
#include <QPointer>
#include <vector>

class MUONDETECTORSHARED TcpConnection : public QObject
{
	Q_OBJECT

public:
	TcpConnection(QString hostName, quint16 port, int verbose = 0, int timeout = 15000,
		int pingInterval = 5000, QObject *parent = 0);
	TcpConnection(int socketDescriptor, int verbose = 0, int timeout = 15000,
		int pingInterval = 5000, QObject *parent = 0);
	~TcpConnection();
	void delay(int millisecondsWait);
	const QString& getPeerAddress() const { return peerAddress; }
	quint16 getPeerPort() const { return peerPort; }
	QTcpSocket* getTcpSocket() { return tcpSocket; }
	uint32_t getNrBytesRead() const { return bytesRead; }
	uint32_t getNrBytesWritten() const { return bytesWritten; }
	time_t firstConnectionTime() const { return firstConnection; }
	
	// void startTimePulser();

signals:
	void madeConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort);
	void connectionTimeout(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort,
		quint32 timeoutTime, quint32 connectionDuration);
//	void connectionTimeout(QString remotePeerAddress, quint16 remotePeerPort,
//		quint32 timeoutTime, quint32 connectionDuration);
	void error(int socketError, const QString message);
	void toConsole(QString data);
	// void stopTimePulser();
	void connected();
	void receivedTcpMessage(TcpMessage tcpMessage);

public slots:
	void makeConnection();
	void receiveConnection();
    void closeConnection(QString closedAddress);
    void closeThisConnection();
	void onReadyRead();
	/*
	  void onTimePulse();
	  bool sendFile(QString fileName = "");
	  bool sendMessage(TcpMessage tcpMessage);
	  bool sendText(const quint16 someCode, QString someText);
	  bool sendCode(const quint16 someCode);
	  bool sendI2CProperties(I2cProperty i2cProperty, bool setProperties);
	  bool sendI2CPropertiesRequest();
	  bool sendGpioRisingEdge(quint8 pin, quint32 tick);
	  bool sendUbxMsgRatesRequest();
	  bool sendUbxMsgRates(QMap<uint16_t,int> msgRateCfgs);
   */
    bool sendTcpMessage(TcpMessage tcpMessage);

private:
	//    bool handleFileTransfer(QString fileName, QByteArray& block, quint16 nextCount);
    bool writeBlock(const QByteArray &block);
	int timeout;
	int verbose;
	int pingInterval;
	int socketDescriptor;
    quint16 blockSize = 0;
    QString peerAddress, localAddress;
	//    quint16 fileCounter = -1;
	//    QFile *file = nullptr;
    QDataStream *in = nullptr;
    QTcpSocket *tcpSocket = nullptr;
	QString hostName;
	quint16 port;
	quint16 peerPort;
	quint16 localPort;
    QPointer<QTimer> t;
	time_t lastConnection;
	time_t firstConnection;
	uint32_t bytesRead=0, bytesWritten=0;
/*
	static std::vector<TcpConnection*> globalConnectionList;
	static uint32_t globalNrBytesRead;
	static uint32_t globalNrBytesWritten;
*/	
};
#endif // TCPCONNECTION_H
