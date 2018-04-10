#include <QtNetwork>
#include <chrono>
#include <QThread>
#include "client.h"

using namespace std;

Client::Client(std::string new_gpsdevname, int new_verbose, bool new_allSats,
	bool new_listSats, bool new_dumpRaw, int new_baudrate, bool new_poll,
    bool new_configGnss, int new_timingCmd, long int new_N,QString serverAddress, quint16 serverPort, QObject *parent)
	: QObject(parent)
{
    //connect(this, &Client::posixTerminate, this, &Client::deleteLater);

	// All the stuff to make the tcp connection work

	// find out IP to connect
	/*QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
	// use the first non-localhost IPv4 address
	for (int i = 0; i < ipAddressesList.size(); ++i) {
		if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
			ipAddressesList.at(i).toIPv4Address()) {
			ipAddress = ipAddressesList.at(i).toString();
			break;
		}
	}*/
	// use localhost for test purposes
	// if we did not find one, use IPv4 localhost

    ipAddress = serverAddress;
    if (ipAddress.isEmpty()||ipAddress == "local"||ipAddress == "localhost") {
		ipAddress = QHostAddress(QHostAddress::LocalHost).toString();
    }
    port = serverPort;
    if (port == 0){
        port = 51508;
    }
    connectToServer();
    /*
    connect(tcpConnection, SIGNAL(error(int, QString)), this, SLOT(displayError(int, QString)));
    //connect(tcpConnection, SIGNAL(stoppedConnection()), this, SLOT(stoppedConnection()));
    connect(tcpConnection, SIGNAL(toConsole(QString)), this, SLOT(toConsole(QString)));
    connect(this, SIGNAL(posixTerminate()), tcpConnection, SLOT(onPosixTerminate()));
    connect(tcpConnection, SIGNAL(finished()), tcpConnection, SLOT(deleteLater()));
	cout<<"Please specify ip\n";
	flush(cout);
	cin>>ipAddress;
	cout<<"Please specify port\n";
	flush(cout);
	cin>>port;
    */


	// All the stuff to make the gps module work
	gpsdevname = new_gpsdevname;
	verbose = new_verbose;
	allSats = new_allSats;
	listSats = new_listSats;
	dumpRaw = new_dumpRaw;
	baudrate = new_baudrate;
	poll = new_poll;
	configGnss = new_configGnss;
	timingCmd = new_timingCmd;
	N = new_N;
    if (verbose > 2){
        cout << "client running in thread " << this->thread() << endl;
    }
    // here is where the magic threading happens look closely
	gps = new Ublox(gpsdevname, baudrate);
    QThread *gpsThread = new QThread();
    gps->moveToThread(gpsThread);
    //connect(this, SIGNAL(posixTerminate()), gps, SLOT(Disconnect()));
    connect(gpsThread, SIGNAL(started()), gps, SLOT(Connect()));
    //connect(gpsThread, SIGNAL(finished()), gps, SLOT(Disconnect()));
    connect(gpsThread, SIGNAL(finished()), gps, SLOT(deleteLater()));
    connect(gps, SIGNAL(UBXCfgError(QString)),
            this, SLOT(displayError(QString)));
    connect(gps, SIGNAL(toConsole(QString)), this, SLOT(toConsole(QString)));
    connect(this, SIGNAL(UBXSetCfgMsg(uint8_t,uint8_t,uint8_t,uint8_t)),
            gps, SLOT(UBXSetCfgMsg(uint8_t,uint8_t,uint8_t,uint8_t)));
    connect(this, SIGNAL(UBXSetCfgMsg(uint16_t,uint8_t,uint8_t)),
            gps, SLOT(UBXSetCfgMsg(uint16_t,uint8_t,uint8_t)));
    connect(this, SIGNAL(UBXSetCfgRate(uint8_t,uint8_t)),
            gps, SLOT(UBXSetCfgRate(uint8_t,uint8_t)));
    connect(gps, SIGNAL(gpsPropertyUpdatedGnss(std::vector<GnssSatellite>,
                                              std::chrono::duration<double>)),
            this, SLOT(gpsPropertyUpdatedGnss(std::vector<GnssSatellite>,
                                              std::chrono::duration<double>)));
    connect(gps, SIGNAL(gpsPropertyUpdatedUint8(uint8_t,
                                                std::chrono::duration<double>, char)),
            this, SLOT(gpsPropertyUpdatedUint8(uint8_t,
                                               std::chrono::duration<double>, char)));
    connect(gps, SIGNAL(gpsPropertyUpdatedUint32(uint32_t,
                                                 std::chrono::duration<double>, char)),
            this, SLOT(gpsPropertyUpdatedUint32(uint32_t,
                                                std::chrono::duration<double>, char)));
    connect(gps, SIGNAL(gpsPropertyUpdatedInt32(int32_t,
                                                std::chrono::duration<double>, char)),
            this, SLOT(gpsPropertyUpdatedInt32(int32_t,
                                               std::chrono::duration<double>, char)));
    gps->setVerbosity(verbose);
    /*if (!gps->Connect()) {
		cout << "failed to open serial interface " << gpsdevname << endl;
		exit(-1);
    }*/
    //gpsThread->start();
    if (configGnss) {
        configGps();
    }
}

void Client::connectToServer(){
    QThread *tcpThread = new QThread();
    if (tcpConnection){
        delete tcpConnection;
    }
    tcpConnection = new TcpConnection(ipAddress, port, verbose);
    tcpConnection->moveToThread(tcpThread);
    connect(tcpThread, &QThread::started, tcpConnection, &TcpConnection::makeConnection);
    connect(tcpThread, &QThread::finished, tcpConnection, &TcpConnection::deleteLater);
    connect(tcpThread, &QThread::finished, tcpThread, &QThread::deleteLater);
    connect(this, &Client::sendFile, tcpConnection, &TcpConnection::sendFile);
    //connect(this, &Client::sendMsg, tcpConnection, &TcpConnection::sendMsg);
    connect(tcpConnection, &TcpConnection::error, this, &Client::displaySocketError);
    connect(tcpConnection, &TcpConnection::toConsole, this, &Client::toConsole);
    //connect(tcpConnection, &TcpConnection::stoppedConnection, this, &Client::stoppedConnection);
    connect(tcpConnection, &TcpConnection::connectionTimeout, this, &Client::connectToServer);
    //connect(this, &Client::posixTerminate, tcpConnection, &TcpConnection::onPosixTerminate);
    tcpThread->start();
}

void Client::configGps() {
	// going to put this in ublox.cpp because it's not really useful here ?

	// 	gps->UBXCfgGNSS();
	// 	gps->UBXCfgNav5();
	// 	gps->UBXMonVer();

	const int measrate = 10;
	emit UBXSetCfgRate(1000 / measrate, 1);
	emit UBXSetCfgMsg(MSG_TIM_TM2, 1, 1);	// TIM-TM2
	emit UBXSetCfgMsg(MSG_TIM_TP, 1, 51);	// TIM-TP
	emit UBXSetCfgMsg(MSG_NAV_TIMEUTC, 1, 20);	// NAV-TIMEUTC
	emit UBXSetCfgMsg(MSG_MON_HW, 1, 47);	// MON-HW
	emit UBXSetCfgMsg(MSG_NAV_SAT, 1, 59);	// NAV-SAT
	emit UBXSetCfgMsg(MSG_NAV_TIMEGPS, 1, 61);	// NAV-TIMEGPS
	emit UBXSetCfgMsg(MSG_NAV_SOL, 1, 67);	// NAV-SOL
	emit UBXSetCfgMsg(MSG_NAV_STATUS, 1, 71);	// NAV-STATUS
	emit UBXSetCfgMsg(MSG_NAV_CLOCK, 1, 89);	// NAV-CLOCK
	emit UBXSetCfgMsg(MSG_MON_TXBUF, 1, 97);	// MON-TXBUF
	emit UBXSetCfgMsg(MSG_NAV_SBAS, 1, 255);	// NAV-SBAS
	emit UBXSetCfgMsg(MSG_NAV_DOP, 1, 101);	// NAV-DOP
	emit UBXSetCfgMsg(MSG_NAV_SVINFO, 1, 49);	// NAV-SVINFO
}

void Client::gpsPropertyUpdatedGnss(std::vector<GnssSatellite> data,
                                std::chrono::duration<double> lastUpdated){
    if (listSats){
        vector<GnssSatellite> sats = data;
        if (!allSats) {
            std::sort(sats.begin(), sats.end(), GnssSatellite::sortByCnr);
            while (sats.back().getCnr() == 0 && sats.size() > 0) sats.pop_back();
        }
        cout << std::chrono::system_clock::now()
                - std::chrono::duration_cast<std::chrono::microseconds>(lastUpdated)
             << "Nr of " << std::string((allSats) ? "visible" : "received")
             << " satellites: " << sats.size() << endl;
        // read nrSats property without evaluation to prevent separate display of this property
        // in the common message poll below
        GnssSatellite::PrintHeader(true);
        for (unsigned int i = 0; i < sats.size(); i++){
            sats[i].Print(i, false);
        }
    }
}
void Client::gpsPropertyUpdatedUint8(uint8_t data, std::chrono::duration<double> updateAge,
                                char propertyName){
    switch (propertyName){
    case 's':
        cout << std::chrono::system_clock::now()
                - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
             << "Nr of available satellites: " << data << endl;
        break;
    case 'e':
        cout << std::chrono::system_clock::now()
                - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
             << "quant error: " << data << " ps" << endl;
        break;
    case 'b':
        cout << std::chrono::system_clock::now()
                - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
             << "TX buf usage: " << data << " %" << endl;
        break;
    case 'p':
        cout << std::chrono::system_clock::now()
                - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
             << "TX buf peak usage: " << data << " %" << endl;
        break;
    default:
        break;
    }
}

void Client::gpsPropertyUpdatedUint32(uint32_t data, chrono::duration<double> updateAge,
                                char propertyName){
    switch (propertyName){
    case 'a':
        cout << std::chrono::system_clock::now()
                - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
             << "time accuracy: " << data << " ns" << endl;
        break;
    case 'c':
        cout << std::chrono::system_clock::now()
                - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
             << "rising edge counter: " << data << endl;
        break;
    default:
        break;
    }
}

void Client::gpsPropertyUpdatedInt32(int32_t data, std::chrono::duration<double> updateAge,
                                char propertyName){
    switch (propertyName){
    case 'd':
        cout << std::chrono::system_clock::now()
                - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
             << "clock drift: " << data << " ns/s" << endl;
        break;
    case 'b':
        cout << std::chrono::system_clock::now()
                - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
             << "clock bias: " << data << " ns" << endl;
        break;
    default:
        break;
    }
}

/*void Client::loop() {
    // not needed anymore since ublox will emit message that something got updated
    // do not use this function!!! (not thread safe unless mutexes in ublox.cpp active
    // but those mutexes are kept out of ublox.cpp since they cost time)
	int i = 0;
	int cycles = 0;
	while (i < N || N == -1) {
		if (dumpRaw) {
			string s;
			int n = gps->ReadBuffer(s);
			if (n > 0) {
				cout << s;
				//cout<<n<<endl;
			}
			continue;
		}
		if (listSats) {
			if (gps->satList.updated) {
				vector<GnssSatellite> sats = gps->satList();
				if (!allSats) {
					std::sort(sats.begin(), sats.end(), GnssSatellite::sortByCnr);
					while (sats.back().getCnr() == 0 && sats.size() > 0) sats.pop_back();
				}
				cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->nrSats.updateAge()) << "Nr of " << std::string((allSats) ? "visible" : "received") << " satellites: " << sats.size() << endl;
				// read nrSats property without evaluation to prevent separate display of this property
				// in the common message poll below
                //int dummy = gps->nrSats();
				GnssSatellite::PrintHeader(true);
                for (unsigned int i = 0; i < sats.size(); i++) sats[i].Print(i, false);
			}
		}

		if (gps->nrSats.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->nrSats.updateAge()) << "Nr of available satellites: " << gps->nrSats() << endl;
		if (gps->timeAccuracy.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->timeAccuracy.updateAge()) << "time accuracy: " << gps->timeAccuracy() << " ns" << endl;
		if (gps->TPQuantErr.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->TPQuantErr.updateAge()) << "quant error: " << gps->TPQuantErr() << " ps" << endl;
		if (gps->noise.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->noise.updateAge()) << "noise: " << gps->noise() << endl;
		if (gps->agc.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->agc.updateAge()) << "agc: " << gps->agc() << endl;
		if (gps->txBufUsage.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->txBufUsage.updateAge()) << "TX buf usage: " << gps->txBufUsage() << " %" << endl;
		if (gps->txBufPeakUsage.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->txBufPeakUsage.updateAge()) << "TX buf peak usage: " << gps->txBufPeakUsage() << " %" << endl;
		if (gps->eventCounter.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->eventCounter.updateAge()) << "rising edge counter: " << gps->eventCounter() << endl;
		if (gps->clkBias.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->clkBias.updateAge()) << "clock bias: " << gps->clkBias() << " ns" << endl;
		if (gps->clkDrift.updated)
			cout << std::chrono::system_clock::now() - std::chrono::duration_cast<std::chrono::microseconds>(gps->clkDrift.updateAge()) << "clock drift: " << gps->clkDrift() << " ns/s" << endl;
		if (gps->getEventFIFOSize()) {
			struct gpsTimestamp ts = gps->getEventFIFOEntry();
			//cout<<ts.rising_time<<" timestamp event";
			if (ts.rising) cout << ts.rising_time << " timestamp event rising " << " accuracy: " << ts.accuracy_ns << " ns  counter: " << ts.counter << endl;
			if (ts.falling) cout << ts.falling_time << " timestamp event falling " << " accuracy: " << ts.accuracy_ns << " ns  counter: " << ts.counter << endl;
			//	  cout<<" accuracy: "<<ts.accuracy_ns<<" ns  counter: "<<ts.counter<<endl;
		}
		cout << flush;
		delay(10000);
		if (++cycles >= 100) {
			cycles = 0;
			i++;
		}
	}
}
*/

void Client::toConsole(QString data) {
    cout <<"client: "<< data << endl;
}

void Client::stoppedConnection(QString hostName, quint16 port, quint32 connectionTimeout, quint32 connectionDuration){
    cout << "stopped connection with " << hostName<<":"<<port<<endl;
    cout<<"connection timeout at "<<connectionTimeout<<"  connection lasted "<<connectionDuration<<"s"<<endl;
}

void Client::displayError(QString message)
{
    cout <<"client: "<< message << endl;
}

void Client::displaySocketError(int socketError, QString message)
{
	switch (socketError) {
	case QAbstractSocket::HostNotFoundError:
		cout << tr("The host was not found. Please check the "
			"host and port settings.\n");
		break;
	case QAbstractSocket::ConnectionRefusedError:
		cout << tr("The connection was refused by the peer. "
			"Make sure the server is running, "
			"and check that the host name and port "
			"settings are correct.\n");
		break;
	default:
		cout << tr("The following error occurred: %1.\n").arg(message);
	}
	flush(cout);
}

void Client::delay(int millisecondsWait)
{
	QEventLoop loop;
	QTimer t;
	t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
	t.start(millisecondsWait);
	loop.exec();
}

void Client::printTimestamp()
{
	std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now();
	std::chrono::microseconds mus = std::chrono::duration_cast<std::chrono::microseconds>(timestamp.time_since_epoch());
	std::chrono::seconds secs = std::chrono::duration_cast<std::chrono::seconds>(mus);
	std::chrono::microseconds subs = mus - secs;


	// 	t1 = std::chrono::system_clock::now();
	// 	t2 = std::chrono::duration_cast<std::chrono::seconds>(t1);



		//double subs=timestamp-(long int)timestamp;
	cout << secs.count() << "." << setw(6) << setfill('0') << subs.count() << " " << setfill(' ');
}

/*
void Client::onPosixTerminateReceived(){
    cout << "posix terminate signal called, safely close application now..."<<endl;
    emit posixTerminate();
}
*/
