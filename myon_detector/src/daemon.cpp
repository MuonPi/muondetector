#include <QtNetwork>
#include <chrono>
#include <QThread>
#include <QNetworkInterface>
#include <daemon.h>
#include <pigpiodhandler.h>
#include <gpio_pin_definitions.h>
#include <ublox_messages.h>
#include <tcpmessage_keys.h>

// for i2cdetect:
extern "C" {
#include <../shared/i2c/custom_i2cdetect.h>
}

using namespace std;


static const QVector<uint16_t> allMsgCfgID({
	//   MSG_CFG_ANT, MSG_CFG_CFG, MSG_CFG_DAT, MSG_CFG_DOSC,
	//   MSG_CFG_DYNSEED, MSG_CFG_ESRC, MSG_CFG_FIXSEED, MSG_CFG_GEOFENCE,
	//   MSG_CFG_GNSS, MSG_CFG_INF, MSG_CFG_ITFM, MSG_CFG_LOGFILTER,
	//   MSG_CFG_MSG, MSG_CFG_NAV5, MSG_CFG_NAVX5, MSG_CFG_NMEA,
	//   MSG_CFG_ODO, MSG_CFG_PM2, MSG_CFG_PMS, MSG_CFG_PRT, MSG_CFG_PWR,
	//   MSG_CFG_RATE, MSG_CFG_RINV, MSG_CFG_RST, MSG_CFG_RXM, MSG_CFG_SBAS,
	//   MSG_CFG_SMGR, MSG_CFG_TMODE2, MSG_CFG_TP5, MSG_CFG_TXSLOT, MSG_CFG_USB
		 MSG_NAV_CLOCK, MSG_NAV_DGPS, MSG_NAV_AOPSTATUS, MSG_NAV_DOP,
		 MSG_NAV_POSECEF, MSG_NAV_POSLLH, MSG_NAV_PVT, MSG_NAV_SBAS, MSG_NAV_SOL,
		 MSG_NAV_STATUS, MSG_NAV_SVINFO, MSG_NAV_TIMEGPS, MSG_NAV_TIMEUTC, MSG_NAV_VELECEF,
		 MSG_NAV_VELNED,
		 MSG_MON_HW, MSG_MON_HW2, MSG_MON_IO, MSG_MON_MSGPP,
		 MSG_MON_RXBUF, MSG_MON_RXR,MSG_MON_TXBUF
	});
// signal handling stuff: put code to execute before shutdown down there
static int setup_unix_signal_handlers()
{
	struct sigaction hup, term, inte;

	hup.sa_handler = Daemon::hupSignalHandler;
	sigemptyset(&hup.sa_mask);
	hup.sa_flags = 0;
	hup.sa_flags |= SA_RESTART;

	if (sigaction(SIGHUP, &hup, 0)) {
		return 1;
	}

	term.sa_handler = Daemon::termSignalHandler;
	sigemptyset(&term.sa_mask);
	term.sa_flags = 0;
	term.sa_flags |= SA_RESTART;

	if (sigaction(SIGTERM, &term, 0)) {
		return 2;
	}

	inte.sa_handler = Daemon::intSignalHandler;
	sigemptyset(&inte.sa_mask);
	inte.sa_flags = 0;
	inte.sa_flags |= SA_RESTART;

	if (sigaction(SIGINT, &inte, 0)) {
		return 3;
	}
	return 0;
}
int Daemon::sighupFd[2];
int Daemon::sigtermFd[2];
int Daemon::sigintFd[2];
void Daemon::handleSigTerm()
{
	snTerm->setEnabled(false);
	char tmp;
	::read(sigtermFd[1], &tmp, sizeof(tmp));

	// do Qt stuff
	if (verbose > 1) {
		cout << "\nSIGTERM received" << endl;
	}
	emit aboutToQuit();
	this->thread()->quit();
	exit(0);
	snTerm->setEnabled(true);
}
void Daemon::handleSigHup()
{
	snHup->setEnabled(false);
	char tmp;
	::read(sighupFd[1], &tmp, sizeof(tmp));

	// do Qt stuff
	if (verbose > 1) {
		cout << "\nSIGHUP received" << endl;
	}
	emit aboutToQuit();
	this->thread()->quit();
	exit(0);
	snHup->setEnabled(true);
}
void Daemon::handleSigInt()
{
	snInt->setEnabled(false);
	char tmp;
	::read(sigintFd[1], &tmp, sizeof(tmp));

	// do Qt stuff
	if (verbose > 1) {
		cout << "\nSIGINT received" << endl;
	}
	if (showin || showout) {
		qDebug() << allMsgCfgID.size();
		qDebug() << msgRateCfgs.size();
		for (QMap<uint16_t, int>::iterator it = msgRateCfgs.begin(); it != msgRateCfgs.end(); it++) {
			qDebug().nospace() << "0x" << hex << (uint8_t)(it.key() >> 8) << " 0x" << hex << (uint8_t)(it.key() & 0xff) << " " << dec << it.value();
		}
	}
	emit aboutToQuit();
	this->thread()->quit();
	exit(0);
	snInt->setEnabled(true);
}


// begin of the Daemon class
Daemon::Daemon(QString new_gpsdevname, int new_verbose, quint8 new_pcaPortMask,
    float* new_dacThresh, float new_biasVoltage, bool bias_ON, bool new_dumpRaw, int new_baudrate,
	bool new_configGnss, QString new_peerAddress, quint16 new_peerPort,
	QString new_daemonAddress, quint16 new_daemonPort, bool new_showout, bool new_showin, QObject *parent)
	: QTcpServer(parent)
{
    qRegisterMetaType<TcpMessage>("TcpMessage");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint8_t>("uint8_t");
    // signal handling
	setup_unix_signal_handlers();
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd)) {
		qFatal("Couldn't create HUP socketpair");
	}

	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd)) {
		qFatal("Couldn't create TERM socketpair");
	}
	if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd)) {
		qFatal("Couldn't createe INT socketpair");
	}

	snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
	connect(snHup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
	snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
	connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
	snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
	connect(snInt, SIGNAL(activated(int)), this, SLOT(handleSigInt()));

	// set all variables

	// general
	verbose = new_verbose;
	if (verbose > 4) {
		cout << "daemon running in thread " << QString("0x%1").arg((int)this->thread()) << endl;
	}

	// for pigpio signals:
	const QVector<unsigned int> gpio_pins({ EVT_AND, EVT_XOR });
	PigpiodHandler* pigHandler = new PigpiodHandler(gpio_pins, this);
	if (pigHandler != nullptr) {
        connect(this, &Daemon::aboutToQuit, pigHandler, &PigpiodHandler::stop);
        connect(pigHandler, &PigpiodHandler::signal, this, &Daemon::sendGpioPinEvent);
	}

	// for i2c devices
	lm75 = new LM75();
	adc = new ADS1115();
	dac = new MCP4728();
	float *tempThresh = new_dacThresh;
	dacThresh.push_back(tempThresh[0]);
	dacThresh.push_back(tempThresh[1]);
	biasVoltage = new_biasVoltage;
    biasON = bias_ON;
    pca = new PCA9536();
    pcaPortMask = new_pcaPortMask;
    setPcaChannel(pcaPortMask);
	for (int i = 0; i < 2; i++) {
		if (dacThresh[i] > 0) {
			dac->setVoltage(i, dacThresh[i]);
		}
	}
	if (biasVoltage > 0) {
		dac->setVoltage(2, biasVoltage);
	}

	// for gps module
	gpsdevname = new_gpsdevname;
	dumpRaw = new_dumpRaw;
	baudrate = new_baudrate;
	configGnss = new_configGnss;
	showout = new_showout;
	showin = new_showin;

	// for separate raspi pin output states
	wiringPiSetup();
	pinMode(UBIAS_EN, 1);
    if (biasON) {
		digitalWrite(UBIAS_EN, 1);
	}
	else {
		digitalWrite(UBIAS_EN, 0);
	}
	pinMode(PREAMP_1, 1);
	digitalWrite(PREAMP_1, 1);
	pinMode(PREAMP_2, 1);
	digitalWrite(PREAMP_2, 1);

	// for tcp connection with fileServer
	peerPort = new_peerPort;
	if (peerPort == 0) {
		peerPort = 51508;
	}
	peerAddress = new_peerAddress;
	if (peerAddress.isEmpty() || peerAddress == "local" || peerAddress == "localhost") {
		peerAddress = QHostAddress(QHostAddress::LocalHost).toString();
	}

	if (new_daemonAddress.isEmpty()) {
		// if not otherwise specified: listen on all available addresses
		daemonAddress = QHostAddress(QHostAddress::Any);
		if (verbose > 1) {
			cout << daemonAddress.toString() << endl;
		}
	}


	daemonPort = new_daemonPort;
	if (daemonPort == 0) {
		// maybe think about other fall back solution
		daemonPort = 51508;
	}
	if (!this->listen(daemonAddress, daemonPort)) {
		cout << tr("Unable to start the server: %1.\n").arg(this->errorString());
	}
	else {
		if (verbose > 4) {
			cout << tr("\nThe server is running on\n\nIP: %1\nport: %2\n\n")
				.arg(daemonAddress.toString()).arg(serverPort());
		}
	}
	flush(cout);

	// start tcp connection and gps module connection
	//connectToServer();
	connectToGps();
	//delay(1000);
	if (configGnss) {
		configGps();
	}
	pollAllUbxMsgRate();
}

Daemon::~Daemon() {
	emit aboutToQuit();
}

void Daemon::connectToGps() {
	// before connecting to gps we have to make sure all other programs are closed
    // and serial echo is off
	if (gpsdevname.isEmpty()) {
		return;
	}
	QProcess prepareSerial;
	QString command = "stty";
	QStringList args = { "-F", "/dev/ttyAMA0", "-echo", "-onlcr" };
	prepareSerial.start(command, args, QIODevice::ReadWrite);
	prepareSerial.waitForFinished();

	// here is where the magic threading happens look closely
	qtGps = new QtSerialUblox(gpsdevname, gpsTimeout, baudrate, dumpRaw, verbose, showout, showin);
	QThread *gpsThread = new QThread();
	qtGps->moveToThread(gpsThread);
	// connect all signals not coming from Daemon to gps
	connect(qtGps, &QtSerialUblox::toConsole, this, &Daemon::gpsToConsole);
	connect(gpsThread, &QThread::started, qtGps, &QtSerialUblox::makeConnection);
	connect(qtGps, &QtSerialUblox::destroyed, gpsThread, &QThread::quit);
	connect(gpsThread, &QThread::finished, gpsThread, &QThread::deleteLater);
	connect(qtGps, &QtSerialUblox::gpsRestart, this, &Daemon::connectToGps);
	connect(this->thread(), &QThread::finished, gpsThread, &QThread::quit);
	// connect all command signals for ublox module here
	connect(this, &Daemon::UBXSetCfgPrt, qtGps, &QtSerialUblox::UBXSetCfgPrt);
	connect(this, &Daemon::UBXSetCfgMsgRate, qtGps, &QtSerialUblox::UBXSetCfgMsgRate);
	connect(this, &Daemon::UBXSetCfgRate, qtGps, &QtSerialUblox::UBXSetCfgRate);
	connect(this, &Daemon::sendPollUbxMsgRate, qtGps, &QtSerialUblox::pollMsgRate);
	connect(this, &Daemon::sendPollUbxMsg, qtGps, &QtSerialUblox::pollMsg);
	connect(qtGps, &QtSerialUblox::UBXReceivedAckNak, this, &Daemon::UBXReceivedAckNak);
	connect(qtGps, &QtSerialUblox::UBXreceivedMsgRateCfg, this, &Daemon::UBXReceivedMsgRateCfg);
	connect(qtGps, &QtSerialUblox::UBXCfgError, this, &Daemon::toConsole);

	// after thread start there will be a signal emitted which starts the qtGps makeConnection function
	gpsThread->start();
}

void Daemon::connectToServer() {
	QThread *tcpThread = new QThread();
	if (tcpConnection != nullptr) {
		delete(tcpConnection);
	}
	tcpConnection = new TcpConnection(peerAddress, peerPort, verbose);
	tcpConnection->moveToThread(tcpThread);
	connect(tcpThread, &QThread::started, tcpConnection, &TcpConnection::makeConnection);
	connect(tcpThread, &QThread::finished, tcpConnection, &TcpConnection::deleteLater);
	connect(tcpThread, &QThread::finished, tcpThread, &QThread::deleteLater);
	connect(this->thread(), &QThread::finished, tcpThread, &QThread::quit);
	connect(tcpConnection, &TcpConnection::error, this, &Daemon::displaySocketError);
	connect(tcpConnection, &TcpConnection::toConsole, this, &Daemon::toConsole);
	connect(tcpConnection, &TcpConnection::connectionTimeout, this, &Daemon::connectToServer);
	//connect(this, &Daemon::sendMsg, tcpConnection, &TcpConnection::sendMsg);
	//connect(this, &Daemon::posixTerminate, tcpConnection, &TcpConnection::onPosixTerminate);
	//connect(tcpConnection, &TcpConnection::stoppedConnection, this, &Daemon::stoppedConnection);
	tcpThread->start();
}

void Daemon::incomingConnection(qintptr socketDescriptor) {
	if (verbose > 4) {
		cout << "incomingConnection" << endl;
	}
	QThread *thread = new QThread();
	TcpConnection *tcpConnection = new TcpConnection(socketDescriptor, verbose);
	tcpConnection->moveToThread(thread);
	connect(thread, &QThread::started, tcpConnection, &TcpConnection::receiveConnection);
	connect(thread, &QThread::finished, tcpConnection, &TcpConnection::deleteLater);
	connect(thread, &QThread::finished, thread, &QThread::deleteLater);
	connect(this->thread(), &QThread::finished, thread, &QThread::quit);
	connect(this, &Daemon::aboutToQuit, tcpConnection, &TcpConnection::closeConnection);
    connect(this, &Daemon::sendTcpMessage, tcpConnection, &TcpConnection::sendTcpMessage);
	connect(tcpConnection, &TcpConnection::receivedTcpMessage, this, &Daemon::receivedTcpMessage);
	connect(tcpConnection, &TcpConnection::toConsole, this, &Daemon::toConsole);
	thread->start();
}

// ALL FUNCTIONS ABOUT TCPMESSAGE SENDING AND RECEIVING
void Daemon::receivedTcpMessage(TcpMessage tcpMessage) {
    if (tcpMessage.getMsgID() == threshSig) {
        uint8_t channel;
        float threshold;
        *(tcpMessage.dStream) >> channel >> threshold;
        setDacThresh(channel, threshold);
        return;
	}
    if (tcpMessage.getMsgID() == threshRequestSig){
        sendDacThresh(0);
        sendDacThresh(1);
        return;
    }
    if (tcpMessage.getMsgID() == biasVoltageSig){
        float voltage;
        *(tcpMessage.dStream) >> voltage;
        setBiasVoltage(voltage);
        return;
    }
    if (tcpMessage.getMsgID() == biasVoltageRequestSig){
        sendBiasVoltage();
        return;
    }
    if (tcpMessage.getMsgID() == biasSig){
        bool status;
        *(tcpMessage.dStream) >> status;
        setBiasStatus(status);
        return;
    }
    if (tcpMessage.getMsgID() == biasRequestSig){
        sendBiasStatus();
    }
    if (tcpMessage.getMsgID() == ubxMsgRateRequest) {
		sendUbxMsgRates();
        return;
	}
    if (tcpMessage.getMsgID() == pcaChannelSig){
        quint8 portMask;
        *(tcpMessage.dStream) >> portMask;
        setPcaChannel((uint8_t)portMask);
        return;
    }
    if (tcpMessage.getMsgID() == pcaChannelRequestSig){
        sendPcaChannel();
        return;
    }
}

void Daemon::sendUbxMsgRates() {
	TcpMessage tcpMessage(ubxMsgRate);
    *(tcpMessage.dStream) << msgRateCfgs;
	emit sendTcpMessage(tcpMessage);
}

void Daemon::sendDacThresh(uint8_t channel) {
    if (channel > 1){ return; }
    TcpMessage tcpMessage(threshSig);
    *(tcpMessage.dStream) << (quint8)channel << dacThresh[(int)channel];
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendGpioPinEvent(uint8_t gpio_pin) {
	TcpMessage tcpMessage(gpioPinSig);
    *(tcpMessage.dStream) << (quint8)gpio_pin;
	emit sendTcpMessage(tcpMessage);
}

void Daemon::sendBiasVoltage(){
    TcpMessage tcpMessage(biasVoltageSig);
    *(tcpMessage.dStream) << biasVoltage;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendBiasStatus(){
    TcpMessage tcpMessage(biasSig);
    *(tcpMessage.dStream) << biasON;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendPcaChannel(){
    TcpMessage tcpMessage(pcaChannelSig);
    *(tcpMessage.dStream) << (quint8)pcaPortMask;
    emit sendTcpMessage(tcpMessage);
}

// ALL FUNCTIONS ABOUT SETTINGS FOR THE I2C-DEVICES (DAC, ADC, PCA...)
void Daemon::setPcaChannel(uint8_t channel) {
    pcaPortMask = channel;
    if (verbose > 1){
        qDebug() << "change pcaPortMask to " << channel;
    }
	if (channel > 3) {
        cout << "there is no channel > 3" << endl;
		return;
	}
	if (!pca) {
		return;
	}
	pca->setOutputPorts(channel);
	pca->setOutputState(channel);
    sendPcaChannel();
}

void Daemon::setBiasVoltage(float voltage) {
    biasVoltage = voltage;
    if (verbose > 1){
        qDebug() << "change biasVoltage to " << voltage;
    }
    dac->setVoltage(2, voltage);
    sendBiasVoltage();
}

void Daemon::setBiasStatus(bool status){
    biasON = status;
    if (verbose > 1){
        qDebug() << "change biasStatus to " << status;
    }
    if (status) {
        digitalWrite(UBIAS_EN, 1);
    }
    else {
        digitalWrite(UBIAS_EN, 0);
    }
    sendBiasStatus();
}

void Daemon::setDacThresh(uint8_t channel, float threshold) {
    if (threshold < 0 || channel > 1) { return; }
    if (threshold > 40.96){
        threshold = 40.96;
    }
    if (verbose > 1){
        qDebug() << "change dacThresh " << channel << " to " << threshold;
    }
    dacThresh[channel] = threshold;
    dac->setVoltage(channel, threshold);
    sendDacThresh(channel);
}

// ALL FUNCTIONS ABOUT UBLOX GPS MODULE
void Daemon::configGps() {
	// set up ubx as only outPortProtocol
	//emit UBXSetCfgPrt(1,1); // enables on UART port (1) only the UBX protocol
	emit UBXSetCfgPrt(1, PROTO_UBX);
	// deactivate all NMEA messages: (port 6 means ALL ports)
	// not needed because of deactivation of all NMEA messages with "UBXSetCfgPrt"
//    msgRateCfgs.insert(MSG_NMEA_DTM,0);
//    // msgRateCfgs.insert(MSG_NMEA_GBQ,0);
//    msgRateCfgs.insert(MSG_NMEA_GBS,0);
//    msgRateCfgs.insert(MSG_NMEA_GGA,0);
//    msgRateCfgs.insert(MSG_NMEA_GLL,0);
//    // msgRateCfgs.insert(MSG_NMEA_GLQ,0);
//    // msgRateCfgs.insert(MSG_NMEA_GNQ,0);
//    msgRateCfgs.insert(MSG_NMEA_GNS,0);
//    // msgRateCfgs.insert(MSG_NMEA_GPQ,0);
//    msgRateCfgs.insert(MSG_NMEA_GRS,0);
//    msgRateCfgs.insert(MSG_NMEA_GSA,0);
//    msgRateCfgs.insert(MSG_NMEA_GST,0);
//    msgRateCfgs.insert(MSG_NMEA_GSV,0);
//    msgRateCfgs.insert(MSG_NMEA_RMC,0);
//    // msgRateCfgs.insert(MSG_NMEA_TXT,0);
//    msgRateCfgs.insert(MSG_NMEA_VLW,0);
//    msgRateCfgs.insert(MSG_NMEA_VTG,0);
//    msgRateCfgs.insert(MSG_NMEA_ZDA,0);
//    msgRateCfgs.insert(MSG_NMEA_POSITION,0);
//    emit UBXSetCfgMsgRate(MSG_NMEA_DTM,6,0);
//    // has no output msg MSG_NMEA_GBQ
//    emit UBXSetCfgMsgRate(MSG_NMEA_GBS,6,0);
//    emit UBXSetCfgMsgRate(MSG_NMEA_GGA,6,0);
//    emit UBXSetCfgMsgRate(MSG_NMEA_GLL,6,0);
//    // has no output msg MSG_NMEA_GLQ
//    // has no output msg MSG_NMEA_GNQ
//    emit UBXSetCfgMsgRate(MSG_NMEA_GNS,6,0);
//    // has no output msg MSG_NMEA_GPQ
//    emit UBXSetCfgMsgRate(MSG_NMEA_GRS,6,0);
//    emit UBXSetCfgMsgRate(MSG_NMEA_GSA,6,0);
//    emit UBXSetCfgMsgRate(MSG_NMEA_GST,6,0);
//    emit UBXSetCfgMsgRate(MSG_NMEA_GSV,6,0);
//    emit UBXSetCfgMsgRate(MSG_NMEA_RMC,6,0);
//    // is not configured through MSG_CFG_MSG but through UBX-CFG-INF!!!  (MSG_NMEA_TXT)
//    //emit UBXSetCfgMsgRate(MSG_NMEA_VLW,6,0); don't know why this does not work, probably not supported anymore
//    emit UBXSetCfgMsgRate(MSG_NMEA_VTG,6,0);
//    emit UBXSetCfgMsgRate(MSG_NMEA_ZDA,6,0);
//    emit UBXSetCfgMsgRate(MSG_NMEA_POSITION,6,0);

	// set protocol configuration for ports
	// msgRateCfgs: -1 means unknown, 0 means off, some positive value means update time
	const int measrate = 10;
	msgRateCfgs.insert(MSG_CFG_RATE, measrate);
	msgRateCfgs.insert(MSG_TIM_TM2, 1);
	msgRateCfgs.insert(MSG_TIM_TP, 51);
	msgRateCfgs.insert(MSG_NAV_TIMEUTC, 20);
	msgRateCfgs.insert(MSG_MON_HW, 47);
	// msgRateCfgs.insert(MSG_NAV_SAT, 59);
	msgRateCfgs.insert(MSG_NAV_TIMEGPS, 61);
	msgRateCfgs.insert(MSG_NAV_SOL, 67);
	msgRateCfgs.insert(MSG_NAV_STATUS, 71);
	msgRateCfgs.insert(MSG_NAV_CLOCK, 89);
	msgRateCfgs.insert(MSG_MON_TXBUF, 97);
	msgRateCfgs.insert(MSG_NAV_SBAS, 255);
	msgRateCfgs.insert(MSG_NAV_DOP, 101);
	msgRateCfgs.insert(MSG_NAV_SVINFO, 49);
	emit UBXSetCfgRate(1000 / measrate, 1); // MSG_CFG_RATE

	emit UBXSetCfgMsgRate(MSG_TIM_TM2, 1, 1);	// TIM-TM2
	emit UBXSetCfgMsgRate(MSG_TIM_TP, 1, 51);	// TIM-TP
	emit UBXSetCfgMsgRate(MSG_NAV_TIMEUTC, 1, 20);	// NAV-TIMEUTC
	emit UBXSetCfgMsgRate(MSG_MON_HW, 1, 47);	// MON-HW
	//emit UBXSetCfgMsgRate(MSG_NAV_SAT, 1, 59);	// NAV-SAT (don't know why it does not work,
	// probably also configured with UBX-CFG-INFO...
	emit UBXSetCfgMsgRate(MSG_NAV_TIMEGPS, 1, 61);	// NAV-TIMEGPS
	emit UBXSetCfgMsgRate(MSG_NAV_SOL, 1, 67);	// NAV-SOL
	emit UBXSetCfgMsgRate(MSG_NAV_STATUS, 1, 71);	// NAV-STATUS
	emit UBXSetCfgMsgRate(MSG_NAV_CLOCK, 1, 89);	// NAV-CLOCK
	emit UBXSetCfgMsgRate(MSG_MON_TXBUF, 1, 97);	// MON-TXBUF
	emit UBXSetCfgMsgRate(MSG_NAV_SBAS, 1, 255);	// NAV-SBAS
	emit UBXSetCfgMsgRate(MSG_NAV_DOP, 1, 101);	// NAV-DOP
	emit UBXSetCfgMsgRate(MSG_NAV_SVINFO, 1, 49);	// NAV-SVINFO
	// this poll is for checking the port cfg (which protocols are enabled etc.)
	emit sendPollUbxMsg(MSG_CFG_PRT);
	//emit sendPoll()
}

void Daemon::pollAllUbxMsgRate() {
	for (const auto& elem : allMsgCfgID) {
		emit sendPollUbxMsgRate(elem);
	}
}

void Daemon::UBXReceivedAckNak(uint16_t ackedMsgID, uint16_t ackedCfgMsgID) {
	// the value was already set correctly before by either poll or set,
	// if not acknowledged or timeout we set the value to -1 (unknown/undefined)
	switch (ackedMsgID) {
	case MSG_CFG_MSG:
		msgRateCfgs.insert(ackedCfgMsgID, -1);
		break;
	default:
		break;
	}
}

void Daemon::UBXReceivedMsgRateCfg(uint16_t msgID, uint8_t rate) {
	msgRateCfgs.insert(msgID, rate);
}


void Daemon::gpsPropertyUpdatedGnss(std::vector<GnssSatellite> data,
	std::chrono::duration<double> lastUpdated) {
	//if (listSats){
	if (verbose > 3) {
		vector<GnssSatellite> sats = data;
		bool allSats = true;
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
		for (unsigned int i = 0; i < sats.size(); i++) {
			sats[i].Print(i, false);
		}
	}
}
void Daemon::gpsPropertyUpdatedUint8(uint8_t data, std::chrono::duration<double> updateAge,
	char propertyName) {
	switch (propertyName) {
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

void Daemon::gpsPropertyUpdatedUint32(uint32_t data, chrono::duration<double> updateAge,
	char propertyName) {
	switch (propertyName) {
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

void Daemon::gpsPropertyUpdatedInt32(int32_t data, std::chrono::duration<double> updateAge,
	char propertyName) {
	switch (propertyName) {
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

void Daemon::toConsole(QString data) {
	cout << data << endl;
}

void Daemon::gpsToConsole(QString data) {
	cout << data << flush;
}

void Daemon::gpsConnectionError() {

}

// ALL OTHER UTITLITY FUNCTIONS
void Daemon::stoppedConnection(QString hostName, quint16 port, quint32 connectionTimeout, quint32 connectionDuration) {
	cout << "stopped connection with " << hostName << ":" << port << endl;
	cout << "connection timeout at " << connectionTimeout << "  connection lasted " << connectionDuration << "s" << endl;
}

void Daemon::displayError(QString message)
{
	cout << "Daemon: " << message << endl;
}

void Daemon::displaySocketError(int socketError, QString message)
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

void Daemon::delay(int millisecondsWait)
{
	QEventLoop loop;
	QTimer t;
	t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
	t.start(millisecondsWait);
	loop.exec();
}

void Daemon::printTimestamp()
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

// some signal handling stuff
void Daemon::hupSignalHandler(int)
{
	char a = 1;
	::write(sighupFd[0], &a, sizeof(a));
}

void Daemon::termSignalHandler(int)
{
	char a = 1;
	::write(sigtermFd[0], &a, sizeof(a));
}
void Daemon::intSignalHandler(int) {
	char a = 1;
	::write(sigintFd[0], &a, sizeof(a));
}
