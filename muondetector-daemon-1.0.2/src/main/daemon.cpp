#include <QtNetwork>
#include <chrono>
#include <time.h>
#include <QThread>
#include <QNetworkInterface>
#include <daemon.h>
#include <gpio_pin_definitions.h>
//#include <calib_struct.h>
#include <ublox_messages.h>
#include <tcpmessage_keys.h>
#include <iomanip>      // std::setfill, std::setw
#include <locale>
#include <iostream>

// for i2cdetect:
extern "C" {
#include <custom_i2cdetect.h>
}

#define DAC_BIAS 2 // channel of the dac where bias voltage is set
#define DAC_TH1 0 // channel of the dac where threshold 1 is set
#define DAC_TH2 1 // channel of the dac where threshold 2 is set

// REMEMBER: "emit" keyword is just syntactic sugar and not needed AT ALL ... learned it after 1 year *clap* *clap*

using namespace std;

int64_t msecdiff(timespec &ts, timespec &st){
    int64_t diff;
    diff = (int64_t)ts.tv_sec - (int64_t)st.tv_sec;
    diff *= 1000;
    diff += ((int64_t)ts.tv_nsec-(int64_t)st.tv_nsec)/1000000;
    return diff;
}

static QVector<uint16_t> allMsgCfgID({
	//   MSG_CFG_ANT, MSG_CFG_CFG, MSG_CFG_DAT, MSG_CFG_DOSC,
	//   MSG_CFG_DYNSEED, MSG_CFG_ESRC, MSG_CFG_FIXSEED, MSG_CFG_GEOFENCE,
	//   MSG_CFG_GNSS, MSG_CFG_INF, MSG_CFG_ITFM, MSG_CFG_LOGFILTER,
	//   MSG_CFG_MSG, MSG_CFG_NAV5, MSG_CFG_NAVX5, MSG_CFG_NMEA,
	//   MSG_CFG_ODO, MSG_CFG_PM2, MSG_CFG_PMS, MSG_CFG_PRT, MSG_CFG_PWR,
	//   MSG_CFG_RATE, MSG_CFG_RINV, MSG_CFG_RST, MSG_CFG_RXM, MSG_CFG_SBAS,
	//   MSG_CFG_SMGR, MSG_CFG_TMODE2, MSG_CFG_TP5, MSG_CFG_TXSLOT, MSG_CFG_USB
		 MSG_TIM_TM2,MSG_TIM_TP,
		 MSG_NAV_CLOCK, MSG_NAV_DGPS, MSG_NAV_AOPSTATUS, MSG_NAV_DOP,
		 MSG_NAV_POSECEF, MSG_NAV_POSLLH, MSG_NAV_PVT, MSG_NAV_SBAS, MSG_NAV_SOL,
		 MSG_NAV_STATUS, MSG_NAV_SVINFO, MSG_NAV_TIMEGPS, MSG_NAV_TIMEUTC, MSG_NAV_VELECEF,
		 MSG_NAV_VELNED, /* MSG_NAV_SAT, */
		 MSG_MON_HW, MSG_MON_HW2, MSG_MON_IO, MSG_MON_MSGPP,
         MSG_MON_RXBUF, MSG_MON_RXR, MSG_MON_TXBUF
	});


QDataStream& operator << (QDataStream& out, const CalibStruct& calib)
{
	out << QString::fromStdString(calib.name) << QString::fromStdString(calib.type)
	 << (quint16)calib.address << QString::fromStdString(calib.value);
    return out;
}

QDataStream & operator >> (QDataStream& in, CalibStruct& calib)
{
	QString s1,s2,s3;
	quint16 u;
	in >> s1 >> s2;
	in >> u;
	in >> s3;
	calib.name = s1.toStdString();
	calib.type = s2.toStdString();
	calib.address = (uint16_t)u;
	calib.value = s3.toStdString();
	return in;
}

QDataStream& operator << (QDataStream& out, const GnssSatellite& sat)
{
	out << sat.fGnssId << sat.fSatId << sat.fCnr << sat.fElev << sat.fAzim
		<< sat.fPrRes << sat.fQuality << sat.fHealth << sat.fOrbitSource
		<< sat.fUsed << sat.fDiffCorr << sat.fSmoothed;
	return out;
}

QDataStream& operator << (QDataStream& out, const UbxTimePulseStruct& tp)
{
	out << tp.tpIndex << tp.version << tp.antCableDelay << tp.rfGroupDelay
	<< tp.freqPeriod << tp.freqPeriodLock << tp.pulseLenRatio << tp.pulseLenRatioLock
	<< tp.userConfigDelay << tp.flags;
    return out;
}

QDataStream& operator >> (QDataStream& in, UbxTimePulseStruct& tp)
{
    in >> tp.tpIndex >> tp.version >> tp.antCableDelay >> tp.rfGroupDelay
    >> tp.freqPeriod >> tp.freqPeriodLock >> tp.pulseLenRatio >> tp.pulseLenRatioLock
    >> tp.userConfigDelay >> tp.flags;
    return in;
}

QDataStream& operator << (QDataStream& out, const Histogram& h)
{
	out << QString::fromStdString(h.fName) << h.fMin << h.fMax << h.fUnderflow << h.fOverflow << h.fNrBins;
	for (int i=0; i<h.fNrBins; i++) {
		out << h.getBinContent(i);
	}
	out << QString::fromStdString(h.fUnit);
    return out;
}

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
	exit(0);
	snInt->setEnabled(true);
}


// begin of the Daemon class
Daemon::Daemon(QString username, QString password, QString new_gpsdevname, int new_verbose, quint8 new_pcaPortMask,
    float *new_dacThresh, float new_biasVoltage, bool bias_ON, bool new_dumpRaw, int new_baudrate,
    bool new_configGnss, QString new_peerAddress, quint16 new_peerPort,
    QString new_daemonAddress, quint16 new_daemonPort, bool new_showout, bool new_showin, QObject *parent)
	: QTcpServer(parent)
{

	// first, we must set the locale to be independent of the number format of the system's locale.
	// We rely on parsing floating point numbers with a decimal point (not a komma) which might fail if not setting the classic locale
	//	std::locale::global( std::locale( std::cout.getloc(), new punct_facet<char, '.'>) ) );
	//	std::locale mylocale( std::locale( std::cout.getloc(), new punct_facet<char, '.'>) );
	std::locale::global(std::locale::classic());

    qRegisterMetaType<TcpMessage>("TcpMessage");
    qRegisterMetaType<GeodeticPos>("GeodeticPos");
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<int8_t>("int8_t");
    qRegisterMetaType<CalibStruct>("CalibStruct");
	qRegisterMetaType<std::vector<GnssSatellite>>("std::vector<GnssSatellite>");
	qRegisterMetaType<std::vector<GnssConfigStruct>>("std::vector<GnssConfigStruct>");
	qRegisterMetaType<std::chrono::duration<double>>("std::chrono::duration<double>");
	qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<LogParameter>("LogParameter");
    qRegisterMetaType<UbxTimePulseStruct>("UbxTimePulseStruct");
    qRegisterMetaType<UbxDopStruct>("UbxDopStruct");
    qRegisterMetaType<timespec>("timespec");
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

    // create fileHandler
    QThread *fileHandlerThread = new QThread();

    fileHandler = new FileHandler(username, password);
    fileHandler->moveToThread(fileHandlerThread);
    connect(this, &Daemon::aboutToQuit, fileHandler, &FileHandler::deleteLater);
    connect(this, &Daemon::logParameter, fileHandler, &FileHandler::onReceivedLogParameter);
    connect(fileHandlerThread, &QThread::finished, fileHandlerThread, &QThread::deleteLater);
    fileHandlerThread->start();

	// general
	verbose = new_verbose;
	if (verbose > 4) {
        cout << "daemon running in thread " << QString("0x%1").arg((intptr_t)this->thread()) << endl;
    }

	// for pigpio signals:
	const QVector<unsigned int> gpio_pins({ EVT_AND, EVT_XOR, /*ADC_READY,*/ TIMEPULSE });
    pigHandler = new PigpiodHandler(gpio_pins);
    QThread *pigThread = new QThread();
    pigHandler->moveToThread(pigThread);
    connect(this, &Daemon::aboutToQuit, pigHandler, &PigpiodHandler::stop);
    connect(this, &Daemon::GpioSetOutput, pigHandler, &PigpiodHandler::setOutput);
    connect(this, &Daemon::GpioSetInput, pigHandler, &PigpiodHandler::setInput);
    connect(this, &Daemon::GpioSetPullUp, pigHandler, &PigpiodHandler::setPullUp);
    connect(this, &Daemon::GpioSetPullDown, pigHandler, &PigpiodHandler::setPullDown);
	connect(this, &Daemon::GpioSetState, pigHandler, &PigpiodHandler::setGpioState);
    connect(pigHandler, &PigpiodHandler::signal, this, &Daemon::sendGpioPinEvent);
    connect(pigHandler, &PigpiodHandler::samplingTrigger, this, &Daemon::sampleAdc0Event);
	connect(pigHandler, &PigpiodHandler::eventInterval, this, [this](quint64 nsecs) { eventIntervalHisto.fill(1e-6*nsecs); } );
	connect(pigHandler, &PigpiodHandler::eventInterval, this, [this](quint64 nsecs) 
	{ if (nsecs/1000<=eventIntervalShortHisto.getMax()) eventIntervalShortHisto.fill((double)nsecs/1000.); } );
	connect(pigHandler, &PigpiodHandler::timePulseDiff, this, [this](qint32 usecs) 
	{ 	checkRescaleHisto(tpTimeDiffHisto, usecs);
		tpTimeDiffHisto.fill((double)usecs);
		/*cout<<"TP time diff: "<<usecs<<" us"<<endl;*/
	} );
	
	struct timespec ts_res;
	clock_getres(CLOCK_REALTIME, &ts_res);
	if (verbose) {
		cout<<"the timing resolution of the system clock is "<<ts_res.tv_nsec<<" ns"<<endl;
	}
	
    /* looks good but using QPointer should be safer
     * connect(pigHandler, &PigpiodHandler::destroyed, this, [this](){pigHandler = nullptr;});
    */
    // test if "aboutToQuit" is called everytime
    //connect(this, &Daemon::aboutToQuit, [this](){this->toConsole("aboutToQuit called and signal emitted\n");});
    connect(pigThread, &QThread::finished, pigHandler, &PigpiodHandler::deleteLater);
    timespec_get(&lastRateInterval, TIME_UTC);
    startOfProgram = lastRateInterval;
    connect(pigHandler, &PigpiodHandler::signal, this, [this](uint8_t gpio_pin){
        rateCounterIntervalActualisation();
        if (gpio_pin==EVT_XOR){
            quint64 value = xorCounts.back();
            xorCounts.pop_back();
            value++;
            xorCounts.push_back(value);
        }
        if (gpio_pin==EVT_AND){
            quint64 value = andCounts.back();
            andCounts.pop_back();
            value++;
            andCounts.push_back(value);
        }
    });
    pigThread->start();
    rateBufferReminder.setInterval(rateBufferInterval);
    rateBufferReminder.setSingleShot(false);
    connect(&rateBufferReminder, &QTimer::timeout, this, &Daemon::onRateBufferReminder);
    rateBufferReminder.start();

	// for i2c devices
	lm75 = new LM75();
	if (lm75->devicePresent()) {
		if (verbose>2) {
			cout<<"LM75 device is present."<<endl;
			cout<<"temperature is "<<lm75->getTemperature()<<" centigrades Celsius"<<endl;
			cout<<"readout took "<<lm75->getLastTimeInterval()<<" ms"<<endl;
		}
	} else {
		cerr<<"LM75 device NOT present!"<<endl;
	}
	adc = new ADS1115();
	if (adc->devicePresent()) {
		adc->setPga(ADS1115::PGA4V);
		//adc->setPga(0, ADS1115::PGA2V);
//		adc->setRate(0x06);  // ADS1115::RATE475
		adc->setRate(0x07);  // ADS1115::RATE860
		adc->setAGC(false);
		if (!adc->setDataReadyPinMode()) {
			cerr<<"error: failed setting data ready pin mode (setting thresh regs)"<<endl;
		}
		
		if (verbose>2) {
			cout<<"ADS1115 device is present."<<endl;
			bool ok=adc->setLowThreshold(0b00000000);
			ok = ok && adc->setHighThreshold(0b10000000);
			if (ok) cout<<"successfully setting threshold registers"<<endl;
			else cerr<<"error: failed setting threshold registers"<<endl;
			cout<<"single ended channels:"<<endl;
			cout<<"ch0: "<<adc->readADC(0)<<" ch1: "<<adc->readADC(1)
			<<" ch2: "<<adc->readADC(2)<<" ch3: "<<adc->readADC(3)<<endl;
			adc->setDiffMode(true);
			cout<<"diff channels:"<<endl;
			cout<<"ch0-1: "<<adc->readADC(0)<<" ch0-3: "<<adc->readADC(1)
			<<" ch1-3: "<<adc->readADC(2)<<" ch2-3: "<<adc->readADC(3)<<endl;
			adc->setDiffMode(false);
			cout<<"readout took "<<adc->getLastTimeInterval()<<" ms"<<endl;
		}
	} else {
		cerr<<"ADS1115 device NOT present!"<<endl;
	}
	

	dac = new MCP4728();
	if (dac->devicePresent()) {
		if (verbose>2) {
			cout<<"MCP4728 device is present."<<endl;
			cout<<"DAC registers / output voltages:"<<endl;
			for (int i=0; i<4; i++) {
				MCP4728::DacChannel dacChannel;
				MCP4728::DacChannel eepromChannel;
				eepromChannel.eeprom=true;
				dac->readChannel(i, dacChannel);
				dac->readChannel(i, eepromChannel);
				cout<<"  ch"<<i<<": "<<dacChannel.value<<" = "<<MCP4728::code2voltage(dacChannel)<<" V"
				"  (stored: "<<eepromChannel.value<<" = "<<MCP4728::code2voltage(eepromChannel)<<" V)"<<endl;
			}
			cout<<"readout took "<<dac->getLastTimeInterval()<<" ms"<<endl;
		}
	} else {
		cerr<<"MCP4728 device NOT present!"<<endl;
	}
	float *tempThresh = new_dacThresh;
	for (int i=std::min(DAC_TH1, DAC_TH2); i<=std::max(DAC_TH1, DAC_TH2); i++) {
		if (tempThresh[i]<0. && dac->devicePresent()) {
			MCP4728::DacChannel dacChannel;
			MCP4728::DacChannel eepromChannel;
			eepromChannel.eeprom=true;
			dac->readChannel(i, dacChannel);
			dac->readChannel(i, eepromChannel);
			tempThresh[i]=MCP4728::code2voltage(dacChannel);
			//tempThresh[i]=code2voltage(eepromChannel);
		}
	}
	dacThresh.push_back(tempThresh[0]);
	dacThresh.push_back(tempThresh[1]);
	
	biasVoltage = new_biasVoltage;
	if (biasVoltage<0. && dac->devicePresent()) {
		MCP4728::DacChannel dacChannel;
		MCP4728::DacChannel eepromChannel;
		eepromChannel.eeprom=true;
		dac->readChannel(DAC_BIAS, dacChannel);
		dac->readChannel(DAC_BIAS, eepromChannel);
		biasVoltage=MCP4728::code2voltage(dacChannel);
		//tempThresh[i]=code2voltage(eepromChannel);
	}
	
    biasON = bias_ON;
    
    // PCA9536 4 bit I/O I2C device used for selecting the UBX timing input
    pca = new PCA9536();
	if (pca->devicePresent()) {
		if (verbose>2) {
			cout<<"PCA9536 device is present."<<endl;
			cout<<" inputs: 0x"<<hex<<(int)pca->getInputState()<<endl;
			cout<<"readout took "<<dec<<pca->getLastTimeInterval()<<" ms"<<endl;
		}
		pca->setOutputPorts(0x03);
		setPcaChannel(new_pcaPortMask);
	} else {
		cerr<<"PCA9536 device NOT present!"<<endl;
	}
    
    if (dac->devicePresent()) {
		if (dacThresh[0] > 0) dac->setVoltage(DAC_TH1, dacThresh[0]);
		if (dacThresh[1] > 0) dac->setVoltage(DAC_TH2, dacThresh[1]);
		if (biasVoltage > 0) dac->setVoltage(DAC_BIAS, biasVoltage);
	}

	// EEPROM 24AA02 type
	eep = new EEPROM24AA02();
	calib = new ShowerDetectorCalib(eep);
	if (eep->devicePresent()) {
//		readEeprom();
		calib->readFromEeprom();
		if (verbose>1) {
			cout<<"eep device is present."<<endl;
			readEeprom();
			calib->readFromEeprom();
			uint64_t id=calib->getSerialID();
			cout<<"unique ID: 0x"<<hex<<id<<dec<<endl;
			
			if (1==0) {
				uint8_t buf[256];
				for (int i=0; i<256; i++) buf[i]=i;
				if (!eep->writeBytes(0, 256, buf)) cerr<<"error: write to eeprom failed!"<<endl;
				if (verbose>2) cout<<"eep write took "<<eep->getLastTimeInterval()<<" ms"<<endl;
				readEeprom();
			}
			if (1==1) {
				calib->printCalibList();

/*
				calib->setCalibItem("VERSION", (uint8_t)1);
				calib->setCalibItem("DATE", (uint32_t)time(NULL));
				calib->setCalibItem("CALIB_FLAGS", (uint8_t)1);
				calib->setCalibItem("FEATURE_FLAGS", (uint8_t)1);
				calib->setCalibItem("RSENSE", (uint16_t)205);
				calib->setCalibItem("COEFF0", (float)3.1415926535);
				calib->setCalibItem("COEFF1", (float)-1.23456e-4);
				calib->setCalibItem("WRITE_CYCLES", (uint32_t)5);
*/
				calib->printCalibList();
				calib->updateBuffer();
				calib->printBuffer();
				//calib->writeToEeprom();
				readEeprom();

			}
		}
	} else {
		cerr<<"eeprom device NOT present!"<<endl;
	}
	
	ubloxI2c = new UbloxI2c(0x42);
	if (ubloxI2c->devicePresent()) {
		if (verbose>1) {
			cout<<"ublox I2C device interface is present."<<endl;
			uint16_t bufcnt = 0;
			bool ok = ubloxI2c->getTxBufCount(bufcnt);
			if (ok) cout<<"bytes in TX buf: "<<bufcnt<<endl;
/*			unsigned long int argh=0;
			while (argh++<100UL) {
				bufcnt = 0;
				ok = ubloxI2c->getTxBufCount(bufcnt);
				if (ok) cout<<"bytes in TX buf: "<<hex<<bufcnt<<dec<<endl;
				std::string str=ubloxI2c->getData();
				cout<<"string length: "<<str.size()<<endl;
				usleep(200000L);
			}
			ubloxI2c->getData();
			ok = ubloxI2c->getTxBufCount(bufcnt);
			if (ok) cout<<"bytes in TX buf: "<<bufcnt<<endl;
*/
		}
	} else {
		cerr<<"ublox I2C device interface NOT present!"<<endl;
	}
	
	// for diagnostics:
	// print out some i2c device statistics
	if (1==0) {
		cout<<"Nr. of invoked I2C devices (plain count): "<<i2cDevice::getNrDevices()<<endl;
		cout<<"Nr. of invoked I2C devices (gl. device list's size): "<<i2cDevice::getGlobalDeviceList().size()<<endl;
		cout<<"Nr. of bytes read on I2C bus: "<<i2cDevice::getGlobalNrBytesRead()<<endl;
		cout<<"Nr. of bytes written on I2C bus: "<<i2cDevice::getGlobalNrBytesWritten()<<endl;
		cout<<"list of device addresses: "<<endl;
		for (uint8_t i=0; i<i2cDevice::getGlobalDeviceList().size(); i++)
		{
			cout<<(int)i+1<<" 0x"<<hex<<(int)i2cDevice::getGlobalDeviceList()[i]->getAddress()<<" "<<i2cDevice::getGlobalDeviceList()[i]->getTitle();
			if (i2cDevice::getGlobalDeviceList()[i]->devicePresent()) cout<<" present"<<endl;
			else cout<<" missing"<<endl;
		}
		lm75->getCapabilities();
	}
	
	// for gps module
	gpsdevname = new_gpsdevname;
	dumpRaw = new_dumpRaw;
	baudrate = new_baudrate;
	configGnss = new_configGnss;
	showout = new_showout;
	showin = new_showin;

	// for separate raspi pin output states
/*
    emit GpioSetOutput(UBIAS_EN);
	emit GpioSetState(UBIAS_EN, biasON);
*/
	wiringPiSetup();
	pinMode(UBIAS_EN, 1);
    if (biasON) {
		digitalWrite(UBIAS_EN, 1);
	}
	else {
		digitalWrite(UBIAS_EN, 0);
	}

	preampStatus[0]=preampStatus[1]=true;
	pinMode(PREAMP_1, 1);
	digitalWrite(PREAMP_1, preampStatus[0]);
	pinMode(PREAMP_2, 1);
	digitalWrite(PREAMP_2, preampStatus[1]);
	gainSwitch=false;
	pinMode(GAIN_HL, 1);
	digitalWrite(GAIN_HL, gainSwitch);
	
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

	// connect to the regular log timer signal to log several non-regularly polled parameters
	connect(fileHandler, &FileHandler::logIntervalSignal, this, [this]() {
        double xorRate = 0.0;
        if (!xorRatePoints.isEmpty()){
            xorRate = xorRatePoints.back().y();
        }
		logParameter(LogParameter("rateXOR", QString::number(xorRate)+" Hz"));
        double andRate = 0.0;
        if (!andRatePoints.isEmpty()){
            andRate = andRatePoints.back().y();
        }
		logParameter(LogParameter("rateAND", QString::number(andRate)+" Hz"));
		if (lm75 && lm75->devicePresent()) logParameter(LogParameter("temperature", QString::number(lm75->getTemperature())+" degC"));
		if (dac && dac->devicePresent()) {
			logParameter(LogParameter("thresh1", QString::number(dacThresh[0])+" V"));
			logParameter(LogParameter("thresh2", QString::number(dacThresh[1])+" V"));
			logParameter(LogParameter("biasDAC", QString::number(biasVoltage)+" V"));
		}
		if (pca && pca->devicePresent()) logParameter(LogParameter("interruptInputSwitch", "0x"+QString::number(pcaPortMask,16)));
		logBiasValues();
		if (adc && !(adc->getStatus() & i2cDevice::MODE_UNREACHABLE)) {
			logParameter(LogParameter("adcSamplingTime", QString::number(adc->getLastConvTime())+" ms"));
			checkRescaleHisto(adcSampleTimeHisto, adc->getLastConvTime());
			adcSampleTimeHisto.fill(adc->getLastConvTime());
			sendHistogram(adcSampleTimeHisto);
			sendHistogram(pulseHeightHisto);
		}
		sendHistogram(ubxTimeLengthHisto);
		sendHistogram(eventIntervalHisto);
		sendHistogram(eventIntervalShortHisto);
		sendHistogram(ubxTimeIntervalHisto);
		sendHistogram(tpTimeDiffHisto);
	});


	// set up histograms
	setupHistos();

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
    snHup.clear();
    snTerm.clear();
    snInt.clear();
    if (pca!=nullptr){ delete pca; pca = nullptr; }
    if (dac!=nullptr){ delete dac; dac = nullptr; }
    if (adc!=nullptr){ delete adc; adc = nullptr; }
    if (eep!=nullptr){ delete eep; eep = nullptr; }
    if (calib!=nullptr){ delete calib; calib = nullptr; }
    pigHandler.clear();
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
    // connect all signals about quitting
    connect(this, &Daemon::aboutToQuit, qtGps, &QtSerialUblox::deleteLater);
    connect(gpsThread, &QThread::finished, gpsThread, &QThread::deleteLater);
	// connect all signals not coming from Daemon to gps
	connect(qtGps, &QtSerialUblox::toConsole, this, &Daemon::gpsToConsole);
	connect(gpsThread, &QThread::started, qtGps, &QtSerialUblox::makeConnection);
	connect(qtGps, &QtSerialUblox::gpsRestart, this, &Daemon::connectToGps);
	// connect all command signals for ublox module here
	connect(this, &Daemon::UBXSetCfgPrt, qtGps, &QtSerialUblox::UBXSetCfgPrt);
	connect(this, &Daemon::UBXSetCfgMsgRate, qtGps, &QtSerialUblox::UBXSetCfgMsgRate);
	connect(this, &Daemon::UBXSetCfgRate, qtGps, &QtSerialUblox::UBXSetCfgRate);
	connect(this, &Daemon::sendPollUbxMsgRate, qtGps, &QtSerialUblox::pollMsgRate);
	connect(this, &Daemon::sendPollUbxMsg, qtGps, &QtSerialUblox::pollMsg);
	connect(this, &Daemon::sendUbxMsg, qtGps, &QtSerialUblox::enqueueMsg);
	connect(qtGps, &QtSerialUblox::UBXReceivedAckNak, this, &Daemon::UBXReceivedAckNak);
	connect(qtGps, &QtSerialUblox::UBXreceivedMsgRateCfg, this, &Daemon::UBXReceivedMsgRateCfg);
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedGeodeticPos, this, &Daemon::sendUbxGeodeticPos);
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedGnss, this, &Daemon::gpsPropertyUpdatedGnss);
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedUint32, this, &Daemon::gpsPropertyUpdatedUint32);
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedInt32, this, &Daemon::gpsPropertyUpdatedInt32);
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedUint8, this, &Daemon::gpsPropertyUpdatedUint8);
    connect(qtGps, &QtSerialUblox::gpsMonHW, this, &Daemon::gpsMonHWUpdated);
    connect(qtGps, &QtSerialUblox::gpsMonHW2, this, &Daemon::gpsMonHW2Updated);
    connect(qtGps, &QtSerialUblox::gpsVersion, this, &Daemon::UBXReceivedVersion);
	connect(qtGps, &QtSerialUblox::UBXCfgError, this, &Daemon::toConsole);
    connect(qtGps, &QtSerialUblox::UBXReceivedGnssConfig, this, &Daemon::onUBXReceivedGnssConfig);
    connect(qtGps, &QtSerialUblox::UBXReceivedTP5, this, &Daemon::onUBXReceivedTP5);
	connect(this, &Daemon::UBXSetDynModel, qtGps, &QtSerialUblox::setDynamicModel);
	connect(this, &Daemon::resetUbxDevice, qtGps, &QtSerialUblox::UBXReset);
	connect(this, &Daemon::setGnssConfig, qtGps, &QtSerialUblox::onSetGnssConfig);
	connect(this, &Daemon::UBXSetCfgTP5, qtGps, &QtSerialUblox::UBXSetCfgTP5);
	connect(this, &Daemon::UBXSetMinMaxSVs, qtGps, &QtSerialUblox::UBXSetMinMaxSVs);
	connect(this, &Daemon::UBXSetMinCNO, qtGps, &QtSerialUblox::UBXSetMinCNO);
	connect(this, &Daemon::UBXSetAopCfg, qtGps, &QtSerialUblox::UBXSetAopCfg);
    connect(qtGps, &QtSerialUblox::UBXReceivedTimeTM2, this, &Daemon::onUBXReceivedTimeTM2);

    // connect fileHandler related stuff
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedGeodeticPos, this, [this](GeodeticPos pos){
		logParameter(LogParameter("geoLongitude", QString::number(1e-7*pos.lon,'f',5)+" deg"));
		logParameter(LogParameter("geoLatitude", QString::number(1e-7*pos.lat,'f',5)+" deg"));
		logParameter(LogParameter("geoHeightMSL", QString::number(1e-3*pos.hMSL,'f',2)+" m"));
		logParameter(LogParameter("meanGeoHeightMSL", QString::number(geoHeightHisto.getMean(),'f',2)+" m"));
		logParameter(LogParameter("geoHorAccuracy", QString::number(1e-3*pos.hAcc,'f',2)+" m"));
		logParameter(LogParameter("geoVertAccuracy", QString::number(1e-3*pos.vAcc,'f',2)+" m"));
		
		if (1e-3*pos.vAcc<100.) {
			checkRescaleHisto(geoHeightHisto, 1e-3*pos.hMSL);
			geoHeightHisto.fill(1e-3*pos.hMSL /*, heightWeight */);
			this->sendHistogram(geoHeightHisto);
			if (currentDOP.vDOP>0) {
				double heightWeight=100./currentDOP.vDOP;
				checkRescaleHisto(weightedGeoHeightHisto, 1e-3*pos.hMSL);
				weightedGeoHeightHisto.fill(1e-3*pos.hMSL , heightWeight );
				this->sendHistogram(weightedGeoHeightHisto);
			}
		}
		if (1e-3*pos.hAcc<100.) {
			checkRescaleHisto(geoLonHisto, 1e-7*pos.lon);
			geoLonHisto.fill(1e-7*pos.lon /*, heightWeight */);
			checkRescaleHisto(geoLatHisto, 1e-7*pos.lat);
			geoLatHisto.fill(1e-7*pos.lat /*, heightWeight */);
			this->sendHistogram(geoLonHisto);
			this->sendHistogram(geoLatHisto);
		}
		
    });
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedGnss, this, [this](std::vector<GnssSatellite> satlist, std::chrono::duration<double> updateAge){
        int allSats=0, usedSats=0, maxCnr=0;
        for (auto sat : satlist){
            if (sat.fCnr>0) allSats++;
            if (sat.fUsed) usedSats++;
	    if (sat.fCnr>maxCnr) maxCnr=sat.fCnr;
        }
        logParameter(LogParameter("sats",QString("%1").arg(allSats)));
        logParameter(LogParameter("usedSats",QString("%1").arg(usedSats)));
        logParameter(LogParameter("maxCNR",QString("%1").arg(maxCnr)));
    });
    
    connect(qtGps, &QtSerialUblox::gpsMonHW, this, [this](uint16_t noise, uint16_t agc, uint8_t antStatus, uint8_t antPower, uint8_t jamInd, uint8_t flags){
		logParameter(LogParameter("preampNoise", QString::number(-noise)+" dBHz"));
		logParameter(LogParameter("preampAGC", QString::number(agc)));
		logParameter(LogParameter("antennaStatus", QString::number(antStatus)));
		logParameter(LogParameter("antennaPower", QString::number(antPower)));
		logParameter(LogParameter("jammingLevel", QString::number(jamInd/2.55,'f',1)+" %"));
    });

    connect(qtGps, &QtSerialUblox::UBXReceivedDops, this, [this](const UbxDopStruct& dops){
	currentDOP=dops;
	logParameter(LogParameter("positionDOP", QString::number(dops.pDOP/100.)));
	logParameter(LogParameter("timeDOP", QString::number(dops.tDOP/100.)));
    });

    
    if (fileHandler != nullptr){
        connect(qtGps, &QtSerialUblox::timTM2, fileHandler, &FileHandler::writeToDataFile);
    }
	// after thread start there will be a signal emitted which starts the qtGps makeConnection function
	gpsThread->start();
}

void Daemon::connectToServer() {
	QThread *tcpThread = new QThread();
    if (!tcpConnection.isNull()) {
        tcpConnection.clear();
	}
	tcpConnection = new TcpConnection(peerAddress, peerPort, verbose);
	tcpConnection->moveToThread(tcpThread);
	connect(tcpThread, &QThread::started, tcpConnection, &TcpConnection::makeConnection);
	connect(tcpThread, &QThread::finished, tcpConnection, &TcpConnection::deleteLater);
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
    // connect all signals about quitting
    connect(this, &Daemon::aboutToQuit, tcpConnection, &TcpConnection::closeThisConnection);
    connect(this, &Daemon::closeConnection, tcpConnection, &TcpConnection::closeConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    // connect all other signals
    connect(thread, &QThread::started, tcpConnection, &TcpConnection::receiveConnection);
    connect(this, &Daemon::sendTcpMessage, tcpConnection, &TcpConnection::sendTcpMessage);
	connect(tcpConnection, &TcpConnection::receivedTcpMessage, this, &Daemon::receivedTcpMessage);
	connect(tcpConnection, &TcpConnection::toConsole, this, &Daemon::toConsole);
//	connect(tcpConnection, &TcpConnection::madeConnection, this, [this](QString, quint16, QString , quint16) { emit sendPollUbxMsg(MSG_MON_VER); });
	thread->start();
//	madeConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort);
	
	
	emit sendPollUbxMsg(MSG_MON_VER);
	emit sendPollUbxMsg(MSG_CFG_GNSS);
	emit sendPollUbxMsg(MSG_CFG_NAV5);
	emit sendPollUbxMsg(MSG_CFG_TP5);
	emit sendPollUbxMsg(MSG_CFG_NAVX5);

	sendBiasStatus();
	sendBiasVoltage();
	sendDacThresh(0);
	sendDacThresh(1);

	pollAllUbxMsgRate();
}


void Daemon::setupHistos() {
	geoHeightHisto=Histogram("geoHeight",200,0.,199.);
	geoHeightHisto.setUnit("m");
	geoLonHisto=Histogram("geoLongitude",200,0.,0.003);
	geoLonHisto.setUnit("deg");
	geoLatHisto=Histogram("geoLatitude",200,0.,0.003);
	geoLatHisto.setUnit("deg");
	weightedGeoHeightHisto=Histogram("weightedGeoHeight",200,0.,199.);
	weightedGeoHeightHisto.setUnit("m");
	pulseHeightHisto=Histogram("pulseHeight",200,0.,4.0);
	pulseHeightHisto.setUnit("V");
	adcSampleTimeHisto=Histogram("adcSampleTime",100,0.,9.9);
	adcSampleTimeHisto.setUnit("ms");
	ubxTimeLengthHisto=Histogram("UbxEventLength",100,50.,149.);
	ubxTimeLengthHisto.setUnit("ns");
	eventIntervalHisto=Histogram("gpioEventInterval",400,0.,1100.);
	eventIntervalHisto.setUnit("ms");
	eventIntervalShortHisto=Histogram("gpioEventIntervalShort",50,0.,49.);
	eventIntervalShortHisto.setUnit("us");
	ubxTimeIntervalHisto=Histogram("UbxEventInterval",200,0.,1100.);
	ubxTimeIntervalHisto.setUnit("ms");
	tpTimeDiffHisto=Histogram("TPTimeDiff",200,-999.,1000.);
	tpTimeDiffHisto.setUnit("us");
}

void Daemon::rescaleHisto(Histogram& hist, double center, double width) {
	hist.setMin(center-width/2.);
	hist.setMax(center+width/2.);
	hist.clear();
}

void Daemon::rescaleHisto(Histogram& hist, double center) {
	double width=hist.getMax()-hist.getMin();
	rescaleHisto(hist, center, width);
}

void Daemon::checkRescaleHisto(Histogram& hist, double newValue) {
	// Strategy: check if more than 10% of all entries in underflow/overflow
	// set new center to old mean
	// set center to newValue if histo empty or only underflow/overflow filled
	// histo will not be filled with supplied value, it has to be done externally
	double entries=hist.getEntries();
	// do nothing if histo is empty
	if (entries<1.) {
		return;
		rescaleHisto(hist, newValue);
	}
	double ufl=hist.getUnderflow();
	double ofl=hist.getOverflow();
	entries-=ufl+ofl;
	if (ufl>0. && ofl>0. && (ufl+ofl)>0.05*entries) {
		// range is too small, onderflow and overflow have more than 5% of all entries
		double range=hist.getMax()-hist.getMin();
		rescaleHisto(hist, hist.getMean(), 1.1*range);
	} else if (ufl>0.05*entries) {
//		if (entries<1.) rescaleHisto(hist, hist.getMin()-(hist.getMax()-hist.getMin())/2.);
		if (entries<1.) rescaleHisto(hist, newValue);
		else rescaleHisto(hist, hist.getMean());
	} else if (ofl>0.05*entries) {
//		if (entries<1.) rescaleHisto(hist, hist.getMax()+(hist.getMax()-hist.getMin())/2.);
		if (entries<1.) rescaleHisto(hist, newValue);
		else rescaleHisto(hist, hist.getMean());
	}
}

// ALL FUNCTIONS ABOUT TCPMESSAGE SENDING AND RECEIVING
void Daemon::receivedTcpMessage(TcpMessage tcpMessage) {
    quint16 msgID = tcpMessage.getMsgID();
    if (msgID == threshSig) {
        uint8_t channel;
        float threshold;
        *(tcpMessage.dStream) >> channel >> threshold;
        if (threshold<0.001)
			cout<<"Warning: setting DAC "<<channel<<" to 0!"<<endl;
        else setDacThresh(channel, threshold);
        return;
	}
    if (msgID == threshRequestSig){
        sendDacThresh(DAC_TH1);
        sendDacThresh(DAC_TH2);
        return;
    }
    if (msgID == biasVoltageSig){
        float voltage;
        *(tcpMessage.dStream) >> voltage;
        setBiasVoltage(voltage);
        pulseHeightHisto.clear();
        return;
    }
    if (msgID == biasVoltageRequestSig){
        sendBiasVoltage();
        return;
    }
    if (msgID == biasSig){
        bool status;
        *(tcpMessage.dStream) >> status;
        setBiasStatus(status);
        pulseHeightHisto.clear();
        return;
    }
    if (msgID == biasRequestSig){
        sendBiasStatus();
    }
    if (msgID == preampSig){
        quint8 channel;
        bool status;
        *(tcpMessage.dStream) >> channel >> status;
        if (channel==0) {
			preampStatus[0]=status;
			digitalWrite(PREAMP_1, (uint8_t)status);
			pulseHeightHisto.clear();
		} else if (channel==1) {
			preampStatus[1]=status;
			digitalWrite(PREAMP_2, (uint8_t)status);
		}
        return;
    }
    if (msgID == preampRequestSig){
        sendPreampStatus(0);
        sendPreampStatus(1);
    }
    if (msgID == gainSwitchSig){
        bool status;
        *(tcpMessage.dStream) >> status;
		gainSwitch=status;
		digitalWrite(GAIN_HL, (uint8_t)status);
		pulseHeightHisto.clear();
		return;
    }
    if (msgID == gainSwitchRequestSig){
        sendGainSwitchStatus();
    }
    if (msgID == ubxMsgRateRequest) {
		sendUbxMsgRates();
        return;
	}
    if (msgID == ubxResetSig) {
		uint32_t resetFlags = QtSerialUblox::RESET_WARM | QtSerialUblox::RESET_SW;
		emit resetUbxDevice(resetFlags);
		pollAllUbxMsgRate();
        return;
	}
    if (msgID == ubxConfigureDefaultSig) {
		configGps();
		pollAllUbxMsgRate();
        return;
	}
    if (msgID == ubxMsgRate){
        QMap<uint16_t, int> ubxMsgRates;
        *(tcpMessage.dStream) >> ubxMsgRates;
        setUbxMsgRates(ubxMsgRates);
    }
    if (msgID == pcaChannelSig){
        quint8 portMask;
        *(tcpMessage.dStream) >> portMask;
        setPcaChannel((uint8_t)portMask);
        ubxTimeLengthHisto.clear();
        return;
    }
    if (msgID == pcaChannelRequestSig){
        sendPcaChannel();
        return;
    }
    if (msgID == gpioRateRequestSig){
        quint8 whichRate;
        quint16 number;
        *(tcpMessage.dStream) >> number >> whichRate;
        sendGpioRates(number, whichRate);
    }
    if (msgID == resetRateSig){
        clearRates();
        return;
    }
	if (msgID == dacRequestSig){
		quint8 channel;
		*(tcpMessage.dStream) >> channel;
		MCP4728::DacChannel channelData;
		if (!dac->devicePresent()) return;
		dac->readChannel(channel, channelData);
		float voltage = MCP4728::code2voltage(channelData);
		sendDacReadbackValue(channel, voltage);
    }
    if (msgID == adcSampleRequestSig){
        quint8 channel;
        *(tcpMessage.dStream) >> channel;
        sampleAdcEvent(channel);
    }
    if (msgID == temperatureRequestSig){
        getTemperature();
    }
    if (msgID == i2cStatsRequestSig){
        sendI2cStats();
    }
        if (msgID == i2cScanBusRequestSig){
        scanI2cBus();
        sendI2cStats();
    }
    if (msgID == calibRequestSig){
        sendCalib();
    }
    if (msgID == calibWriteEepromSig){
        if (calib!=nullptr) calib->writeToEeprom();
        sendCalib();
    }
    if (msgID == calibSetSig){
        std::vector<CalibStruct> calibs;
        quint8 nrEntries=0;
        *(tcpMessage.dStream) >> nrEntries;
        for (int i=0; i<nrEntries; i++) {
			CalibStruct item;
			*(tcpMessage.dStream) >> item;
			calibs.push_back(item);
		}
		receivedCalibItems(calibs);
    }
    if (msgID == gnssConfigSig){
        std::vector<GnssConfigStruct> configs;
        int nrEntries=0;
        *(tcpMessage.dStream) >> nrEntries;
        for (int i=0; i<nrEntries; i++) {
			GnssConfigStruct config;
			*(tcpMessage.dStream) >> config.gnssId >> config.resTrkCh >>
                config.maxTrkCh >> config.flags;
			configs.push_back(config);
		}
		emit setGnssConfig(configs);
		usleep(150000L);
		emit sendPollUbxMsg(MSG_CFG_GNSS);
    }
    if (msgID == gpsCfgTP5Sig){
        UbxTimePulseStruct tp;
        *(tcpMessage.dStream) >> tp;
        emit UBXSetCfgTP5(tp);
        emit sendPollUbxMsg(MSG_CFG_TP5);
        return;
    }
    if (msgID == quitConnectionSig){
        QString closeAddress;
        *(tcpMessage.dStream) >> closeAddress;
        emit closeConnection(closeAddress);
    }
}

void Daemon::scanI2cBus() {
	for (uint8_t addr=1; addr<0x7f; addr++)
	{
		bool alreadyThere = false;
		for (uint8_t i=0; i<i2cDevice::getGlobalDeviceList().size(); i++) {
			if (addr==i2cDevice::getGlobalDeviceList()[i]->getAddress()) {
				alreadyThere=true;
				break;
			}
		}
		if (alreadyThere) continue;
		i2cDevice* dev = new i2cDevice(addr);
		if (!dev->devicePresent()) delete dev;
	}
}

void Daemon::sendI2cStats() {
	TcpMessage tcpMessage(i2cStatsSig);
	quint8 nrDevices=i2cDevice::getGlobalDeviceList().size();
	quint32 bytesRead = i2cDevice::getGlobalNrBytesRead();
	quint32 bytesWritten = i2cDevice::getGlobalNrBytesWritten();
    *(tcpMessage.dStream) << nrDevices << bytesRead << bytesWritten;

	for (uint8_t i=0; i<i2cDevice::getGlobalDeviceList().size(); i++)
	{
		uint8_t addr = i2cDevice::getGlobalDeviceList()[i]->getAddress();
		QString title = QString::fromStdString(i2cDevice::getGlobalDeviceList()[i]->getTitle());
		uint8_t status = i2cDevice::getGlobalDeviceList()[i]->getStatus();
		*(tcpMessage.dStream) << addr << title << status;
	}
	emit sendTcpMessage(tcpMessage);
}

void Daemon::sendCalib() {
	TcpMessage tcpMessage(calibSetSig);
	bool valid=calib->isValid();
	bool eepValid=calib->isEepromValid();
    quint16 nrPars = calib->getCalibList().size();
    quint64 id = calib->getSerialID();
    *(tcpMessage.dStream) << valid << eepValid << id << nrPars;
    for (int i=0; i<nrPars; i++) {
		*(tcpMessage.dStream) << calib->getCalibItem(i);
	}
	emit sendTcpMessage(tcpMessage);
}

void Daemon::receivedCalibItems(const std::vector<CalibStruct>& newCalibs) {
    for (unsigned int i=0; i<newCalibs.size(); i++) {
		calib->setCalibItem(newCalibs[i].name, newCalibs[i]);
	}
}


void Daemon::sendUbxGeodeticPos(GeodeticPos pos){
    TcpMessage tcpMessage(geodeticPosSig);
    (*tcpMessage.dStream) << pos.iTOW << pos.lon << pos.lat
                          << pos.height << pos.hMSL << pos.hAcc
                          << pos.vAcc;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendHistogram(const Histogram& hist){
    TcpMessage tcpMessage(histogramSig);
    (*tcpMessage.dStream) << hist;
    emit sendTcpMessage(tcpMessage);
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

void Daemon::sendDacReadbackValue(uint8_t channel, float voltage) {
    if (channel > 3){ return; }
    
    TcpMessage tcpMessage(dacReadbackSig);
    *(tcpMessage.dStream) << (quint8)channel << voltage;
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

void Daemon::sendGainSwitchStatus(){
    TcpMessage tcpMessage(gainSwitchSig);
    *(tcpMessage.dStream) << gainSwitch;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendPreampStatus(uint8_t channel) {
    if (channel > 1){ return; }
    TcpMessage tcpMessage(preampSig);
    *(tcpMessage.dStream) << (quint8)channel << preampStatus [channel];
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendPcaChannel(){
    TcpMessage tcpMessage(pcaChannelSig);
    *(tcpMessage.dStream) << (quint8)pcaPortMask;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::rateCounterIntervalActualisation(){
    if (xorCounts.isEmpty()){
        xorCounts.push_back(0);
    }
    if (andCounts.isEmpty()){
        andCounts.push_back(0);
    }
    timespec now;
    timespec_get(&now, TIME_UTC);
    int64_t diff = msecdiff(now,lastRateInterval);
    while(diff>1000){
        xorCounts.push_back(0);
        andCounts.push_back(0);
        while (xorCounts.size()>(120*120)){
            xorCounts.pop_front();
        }
        while (andCounts.size()>(120*120)){
            andCounts.pop_front();
        }
        lastRateInterval.tv_sec += 1;
        diff = msecdiff(now,lastRateInterval);
    }
}

void Daemon::clearRates(){
    xorCounts.clear();
    xorCounts.push_back(0);
    andCounts.clear();
    andCounts.push_back(0);
    xorRatePoints.clear();
    andRatePoints.clear();
}

qreal Daemon::getRateFromCounts(quint8 which_rate){
    rateCounterIntervalActualisation();
    QList<quint64> *counts;
    if (which_rate==XOR_RATE){
        counts = &xorCounts;
    }else if( which_rate == AND_RATE){
        counts = &andCounts;
    }else{
        return -1.0;
    }
    quint64 sum = 0;
    for (auto count : *counts){
        sum += count;
    }
    timespec now;
    timespec_get(&now, TIME_UTC);
    qreal timeInterval = (qreal)(1000*(counts->size()-1)+(qreal)msecdiff(now,lastRateInterval));
    qreal rate = sum/timeInterval*1000;
    return (rate);
}

void Daemon::onRateBufferReminder(){
    qreal secsSinceStart = 0.001*(qreal)msecdiff(lastRateInterval,startOfProgram);
    qreal xorRate = getRateFromCounts(XOR_RATE);
    qreal andRate = getRateFromCounts(AND_RATE);
    QPointF xorPoint(secsSinceStart, xorRate);
    QPointF andPoint(secsSinceStart, andRate);
    xorRatePoints.append(xorPoint);
    andRatePoints.append(andPoint);
    while (xorRatePoints.size()>rateBufferTime/rateBufferInterval){
        xorRatePoints.pop_front();
    }
    while (andRatePoints.size()>rateBufferTime/rateBufferInterval){
        andRatePoints.pop_front();
    }
}

void Daemon::sendGpioRates(int number, quint8 whichRate){
    if (pigHandler==nullptr){
        return;
    }
    TcpMessage tcpMessage(gpioRateSig);
    QVector<QPointF> *ratePoints;
    if (whichRate==XOR_RATE){
         ratePoints = &xorRatePoints;
    }else if(whichRate==AND_RATE){
        ratePoints = &andRatePoints;
    }else{
        return;
    }
    QVector<QPointF> someRates;
    if (number >= ratePoints->size() || number == 0){
        number = ratePoints->size()-1;
    }
    if (!ratePoints->isEmpty()){
        for (unsigned int i = 0; i<number; i++){
            someRates.push_front(ratePoints->at(ratePoints->size()-1-i));
        }
    }
    *(tcpMessage.dStream) << whichRate << someRates;
    emit sendTcpMessage(tcpMessage);
}
void Daemon::sampleAdc0Event(){
    const uint8_t channel=0;
    if (adc==nullptr){
        return;
    }
    if (adc->getStatus() & i2cDevice::MODE_UNREACHABLE) return;
    TcpMessage tcpMessage(adcSampleSig);
    float value = adc->readVoltage(channel);
    *(tcpMessage.dStream) << (quint8)channel << value;
    emit sendTcpMessage(tcpMessage);
    pulseHeightHisto.fill(value);
}

void Daemon::sampleAdcEvent(uint8_t channel){
    if (adc==nullptr){
        return;
    }
    if (adc->getStatus() & i2cDevice::MODE_UNREACHABLE) return;
    TcpMessage tcpMessage(adcSampleSig);
    float value = adc->readVoltage(channel);
    *(tcpMessage.dStream) << (quint8)channel << value;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::getTemperature(){
    if (lm75==nullptr){
        return;
    }
    TcpMessage tcpMessage(temperatureSig);
    if (lm75->getStatus() & i2cDevice::MODE_UNREACHABLE) return;
    float value = lm75->getTemperature();
    *(tcpMessage.dStream) << value;
    emit sendTcpMessage(tcpMessage);
}

// ALL FUNCTIONS ABOUT SETTINGS FOR THE I2C-DEVICES (DAC, ADC, PCA...)
void Daemon::setPcaChannel(uint8_t channel) {
    if (verbose > 1){
        qDebug() << "change pcaPortMask to " << channel;
    }
	if (channel > 3) {
        cout << "there is no channel > 3" << endl;
		return;
	}
	if (!pca || !pca->devicePresent()) {
		return;
	}
    pcaPortMask = channel;
    pca->setOutputState(channel);
    sendPcaChannel();
}

void Daemon::setBiasVoltage(float voltage) {
    biasVoltage = voltage;
    if (verbose > 1){
        qDebug() << "change biasVoltage to " << voltage;
    }
    if (dac && dac->devicePresent()) dac->setVoltage(DAC_BIAS, voltage);
    clearRates();
    sendBiasVoltage();
}

void Daemon::setBiasStatus(bool status){
    biasON = status;
    if (verbose > 1){
        qDebug() << "change biasStatus to " << status;
    }
    //emit GpioSetState(UBIAS_EN, status);

    if (status) {
        digitalWrite(UBIAS_EN, 1);
    }
    else {
        digitalWrite(UBIAS_EN, 0);
    }
    clearRates();
    sendBiasStatus();
}

void Daemon::setDacThresh(uint8_t channel, float threshold) {
    if (threshold < 0 || channel > 1) { return; }
    if (threshold > 4.095){
        threshold = 4.095;
    }
    if (verbose > 1){
        qDebug() << "change dacThresh " << channel << " to " << threshold;
    }
    dacThresh[channel] = threshold;
    clearRates();
    if (dac->devicePresent()) dac->setVoltage(channel, threshold);
    sendDacThresh(channel);
}

bool Daemon::readEeprom()
{
	if (eep==nullptr) return false;
	if (eep->devicePresent()) {
		if (verbose>2) cout<<"eep device is present."<<endl;
	} else {
		cerr<<"eeprom device NOT present!"<<endl;
		return false;
	}
	uint16_t n=256;
	uint8_t buf[256];
	for (int i=0; i<n; i++) buf[i]=0;
	bool retval=(eep->readBytes(0,n,buf)==n);
	cout<<"*** EEPROM content ***"<<endl;
	for (int j=0; j<16; j++) {
		cout<<hex<<std::setfill ('0') << std::setw (2)<<j*16<<": ";
		for (int i=0; i<16; i++) {
			cout<<hex<<std::setfill ('0') << std::setw (2)<<(int)buf[j*16+i]<<" ";
		}
		cout<<endl;
	}
	return retval;
}

void Daemon::setUbxMsgRates(QMap<uint16_t, int>& ubxMsgRates){
    for (QMap<uint16_t, int>::iterator it = ubxMsgRates.begin(); it != ubxMsgRates.end(); it++) {
        emit UBXSetCfgMsgRate(it.key(),1,it.value());
        emit sendPollUbxMsgRate(it.key());
        waitingForAppliedMsgRate++;
    }
}

// ALL FUNCTIONS ABOUT UBLOX GPS MODULE
void Daemon::configGps() {
	// set up ubx as only outPortProtocol
	//emit UBXSetCfgPrt(1,1); // enables on UART port (1) only the UBX protocol
	emit UBXSetCfgPrt(1, PROTO_UBX);

	// set dynamic model: Stationary
	emit UBXSetDynModel(2);
	
	emit UBXSetAopCfg(true);
	
	emit sendPollUbxMsg(MSG_MON_VER);

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
    int measrate = 10;
    // msgRateCfgs.insert(MSG_CFG_RATE, measrate);
    // msgRateCfgs.insert(MSG_TIM_TM2, 1);
    // msgRateCfgs.insert(MSG_TIM_TP, 51);
    // msgRateCfgs.insert(MSG_NAV_TIMEUTC, 20);
    // msgRateCfgs.insert(MSG_MON_HW, 47);
	// msgRateCfgs.insert(MSG_NAV_SAT, 59);
    // msgRateCfgs.insert(MSG_NAV_TIMEGPS, 61);
    // msgRateCfgs.insert(MSG_NAV_SOL, 67);
    // msgRateCfgs.insert(MSG_NAV_STATUS, 71);
    // msgRateCfgs.insert(MSG_NAV_CLOCK, 89);
    // msgRateCfgs.insert(MSG_MON_TXBUF, 97);
    // msgRateCfgs.insert(MSG_NAV_SBAS, 255);
    // msgRateCfgs.insert(MSG_NAV_DOP, 101);
    // msgRateCfgs.insert(MSG_NAV_SVINFO, 49);
    emit UBXSetCfgRate(1000/measrate, 1); // MSG_RATE

	//emit sendPollUbxMsg(MSG_MON_VER);
	emit UBXSetCfgMsgRate(MSG_TIM_TM2, 1, 1);	// TIM-TM2
	emit UBXSetCfgMsgRate(MSG_TIM_TP, 1, 0);	// TIM-TP
	emit UBXSetCfgMsgRate(MSG_NAV_TIMEUTC, 1, 131);	// NAV-TIMEUTC
	emit UBXSetCfgMsgRate(MSG_MON_HW, 1, 47);	// MON-HW
	emit UBXSetCfgMsgRate(MSG_MON_HW2, 1, 49);	// MON-HW
	emit UBXSetCfgMsgRate(MSG_NAV_POSLLH, 1, 127);	// MON-POSLLH
	// probably also configured with UBX-CFG-INFO...
	emit UBXSetCfgMsgRate(MSG_NAV_TIMEGPS, 1, 0);	// NAV-TIMEGPS
	emit UBXSetCfgMsgRate(MSG_NAV_SOL, 1, 167);	// NAV-SOL
	emit UBXSetCfgMsgRate(MSG_NAV_STATUS, 1, 71);	// NAV-STATUS
	emit UBXSetCfgMsgRate(MSG_NAV_CLOCK, 1, 189);	// NAV-CLOCK
	emit UBXSetCfgMsgRate(MSG_MON_TXBUF, 1, 51);	// MON-TXBUF
	emit UBXSetCfgMsgRate(MSG_NAV_SBAS, 1, 0);	// NAV-SBAS
	emit UBXSetCfgMsgRate(MSG_NAV_DOP, 1, 254);	// NAV-DOP
	// this poll is for checking the port cfg (which protocols are enabled etc.)
	emit sendPollUbxMsg(MSG_CFG_PRT);
	emit sendPollUbxMsg(MSG_MON_VER);
	emit sendPollUbxMsg(MSG_MON_VER);
	emit sendPollUbxMsg(MSG_MON_VER);
    emit sendPollUbxMsg(MSG_CFG_GNSS);
	//emit UBXSetMinCNO(5);
	emit sendPollUbxMsg(MSG_CFG_NAVX5);
	emit sendPollUbxMsg(MSG_CFG_ANT);
	emit sendPollUbxMsg(MSG_CFG_TP5);

	configGpsForVersion();
}

void Daemon::configGpsForVersion() {
	if (QtSerialUblox::getProtVersion()<=0.1) return;
	if (QtSerialUblox::getProtVersion()>15.0) {
		if (std::find(allMsgCfgID.begin(), allMsgCfgID.end(), MSG_NAV_SAT)==allMsgCfgID.end()) {
			allMsgCfgID.push_back(MSG_NAV_SAT);
		}
		emit UBXSetCfgMsgRate(MSG_NAV_SAT, 1, 69);	// NAV-SAT 
		emit UBXSetCfgMsgRate(MSG_NAV_SVINFO, 1, 0);
	} else emit UBXSetCfgMsgRate(MSG_NAV_SVINFO, 1, 69);	// NAV-SVINFO
	//cout<<"prot Version: "<<QtSerialUblox::getProtVersion()<<endl;
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
    waitingForAppliedMsgRate--;
    if (waitingForAppliedMsgRate<0){
        waitingForAppliedMsgRate = 0;
    }
    if (waitingForAppliedMsgRate==0){
        sendUbxMsgRates();
    }
}

void Daemon::gpsMonHWUpdated(uint16_t noise, uint16_t agc, uint8_t antStatus, uint8_t antPower, uint8_t jamInd, uint8_t flags)
{
    TcpMessage tcpMessage(gpsMonHWSig);
    (*tcpMessage.dStream) << (quint16)noise << (quint16)agc << (quint8) antStatus
    << (quint8)antPower << (quint8)jamInd << (quint8)flags;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::gpsMonHW2Updated(int8_t ofsI, uint8_t magI, int8_t ofsQ, uint8_t magQ, uint8_t cfgSrc)
{
    TcpMessage tcpMessage(gpsMonHW2Sig);
    (*tcpMessage.dStream) << (qint8)ofsI << (quint8)magI << (qint8)ofsQ
    << (quint8)magQ << (quint8)cfgSrc;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::gpsPropertyUpdatedGnss(std::vector<GnssSatellite> data,
	std::chrono::duration<double> lastUpdated) {
	//if (listSats){
	vector<GnssSatellite> sats = data;
	if (verbose > 3) {
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
    int N=sats.size();
    TcpMessage tcpMessage(gpsSatsSig);
    (*tcpMessage.dStream) << N;
    for (int i=0; i<N; i++) {
		(*tcpMessage.dStream)<< sats [i];
	}
    emit sendTcpMessage(tcpMessage);
}

void Daemon::onUBXReceivedGnssConfig(uint8_t numTrkCh, const std::vector<GnssConfigStruct>& gnssConfigs) {
	if (verbose > 2) {
		// put some verbose output here
	}
    int N=gnssConfigs.size();
    TcpMessage tcpMessage(gnssConfigSig);
    (*tcpMessage.dStream) << (int)numTrkCh << N;
    for (int i=0; i<N; i++) {
		(*tcpMessage.dStream)<<gnssConfigs[i].gnssId<<gnssConfigs[i].resTrkCh<<
		gnssConfigs[i].maxTrkCh<<gnssConfigs[i].flags;
	}
    emit sendTcpMessage(tcpMessage);
}

void Daemon::onUBXReceivedTP5(const UbxTimePulseStruct& tp) {
	if (verbose > 2) {
		// put some verbose output here
	}
    TcpMessage tcpMessage(gpsCfgTP5Sig);
    (*tcpMessage.dStream) << tp;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::gpsPropertyUpdatedUint8(uint8_t data, std::chrono::duration<double> updateAge,
	char propertyName) {
	TcpMessage* tcpMessage;
	switch (propertyName) {
	case 's':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "Nr of available satellites: " << (int)data << endl;
		break;
	case 'e':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "quant error: " << (int)data << " ps" << endl;
		break;
	case 'b':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "TX buf usage: " << (int)data << " %" << endl;
		tcpMessage = new TcpMessage(gpsTxBufSig);
		*(tcpMessage->dStream) << (quint8)data;
		emit sendTcpMessage(*tcpMessage);
		delete tcpMessage;
		logParameter(LogParameter("TXBufUsage", QString::number(data)+" %"));
		break;
	case 'p':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "TX buf peak usage: " << (int)data << " %" << endl;
		tcpMessage = new TcpMessage(gpsTxBufPeakSig);
		*(tcpMessage->dStream) << (quint8)data;
		emit sendTcpMessage(*tcpMessage);
		delete tcpMessage;
		logParameter(LogParameter("maxTXBufUsage", QString::number(data)+" %"));
		break;
	case 'f':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "Fix value: " << (int)data << endl;
		tcpMessage = new TcpMessage(gpsFixSig);
		*(tcpMessage->dStream) << (quint8)data;
		emit sendTcpMessage(*tcpMessage);
		delete tcpMessage;
		logParameter(LogParameter("fixStatus", QString::number(data)));
		break;
	default:
		break;
	}
}

void Daemon::gpsPropertyUpdatedUint32(uint32_t data, chrono::duration<double> updateAge,
	char propertyName) {
	TcpMessage* tcpMessage;
	switch (propertyName) {
	case 'a':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "time accuracy: " << data << " ns" << endl;
		tcpMessage = new TcpMessage(gpsTimeAccSig);
		*(tcpMessage->dStream) << (quint32)data;
		emit sendTcpMessage(*tcpMessage);
		delete tcpMessage;
		logParameter(LogParameter("timeAccuracy", QString::number(data)+" ns"));
		break;
	case 'f':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "frequency accuracy: " << data << " ps/s" << endl;
		tcpMessage = new TcpMessage(gpsFreqAccSig);
		*(tcpMessage->dStream) << (quint32)data;
		emit sendTcpMessage(*tcpMessage);
		delete tcpMessage;
		logParameter(LogParameter("freqAccuracy", QString::number(data)+" ps/s"));
		break;
	case 'u':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "Ublox uptime: " << data << " s" << endl;
		tcpMessage = new TcpMessage(gpsUptimeSig);
		*(tcpMessage->dStream) << (quint32)data;
		emit sendTcpMessage(*tcpMessage);
		delete tcpMessage;
		logParameter(LogParameter("ubloxUptime", QString::number(data)+" s"));
		break;
	case 'c':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "rising edge counter: " << data << endl;
		tcpMessage = new TcpMessage(gpsIntCounterSig);
		*(tcpMessage->dStream) << (quint32)data;
		emit sendTcpMessage(*tcpMessage);
		delete tcpMessage;
		break;
	default:
		break;
	}
}

void Daemon::gpsPropertyUpdatedInt32(int32_t data, std::chrono::duration<double> updateAge,
	char propertyName) {
	switch (propertyName) {
	case 'd':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "clock drift: " << data << " ns/s" << endl;
		logParameter(LogParameter("clockDrift", QString::number(data)+" ns/s"));
		break;
	case 'b':
		if (verbose>2)
			cout << std::chrono::system_clock::now()
				- std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
				<< "clock bias: " << data << " ns" << endl;
		logParameter(LogParameter("clockBias", QString::number(data)+" ns"));
		break;
	default:
		break;
	}
}


void Daemon::UBXReceivedVersion(const QString& swString, const QString& hwString, const QString& protString)
{
    static bool initialVersionInfo = true;
    TcpMessage tcpMessage(gpsVersionSig);
    (*tcpMessage.dStream) << swString << hwString << protString;
    emit sendTcpMessage(tcpMessage);
	logParameter(LogParameter("UBX_SW_Version", swString));
	logParameter(LogParameter("UBX_HW_Version", hwString));
	logParameter(LogParameter("UBX_Prot_Version", protString));
	if (initialVersionInfo) {
		configGpsForVersion();
	}
	initialVersionInfo = false;
}

void Daemon::toConsole(const QString &data) {
	cout << data << endl;
}

void Daemon::gpsToConsole(const QString &data) {
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

void Daemon::logBiasValues()
{
	double v1=0.,v2=0.;
	if (adc && (!(adc->getStatus() & i2cDevice::MODE_UNREACHABLE)) && (adc->getStatus() & (i2cDevice::MODE_NORMAL | i2cDevice::MODE_FORCE))) {
		v1=adc->readVoltage(2);
		v2=adc->readVoltage(3);
		if (calib && calib->getCalibItem("VDIV").name=="VDIV") {
			CalibStruct vdivItem=calib->getCalibItem("VDIV");
			std::istringstream istr(vdivItem.value);
			double vdiv;
			istr >> vdiv;
			vdiv/=100.;
			logParameter(LogParameter("vbias1", QString::number(v1*vdiv)+" V"));
			logParameter(LogParameter("vbias2", QString::number(v2*vdiv)+" V"));
            double ubias=v2*vdiv;
			
			CalibStruct flagItem=calib->getCalibItem("CALIB_FLAGS");
			uint8_t calFlags;
			istr=std::istringstream(flagItem.value);
            istr >> calFlags;
            if (calFlags & CalibStruct::CALIBFLAGS_CURRENT_COEFFS) {
				double islope,ioffs,rsense;
				istr=std::istringstream(calib->getCalibItem("COEFF2").value);
				istr >> ioffs;
				istr=std::istringstream(calib->getCalibItem("COEFF3").value);
				istr >> islope;
				istr=std::istringstream(calib->getCalibItem("RSENSE").value);
				istr >> rsense;
				rsense /= 10.*1000.; // yields Rsense in MOhm
				double icorr = ubias*islope+ioffs;
				double ibias = (v1-v2)*vdiv/rsense-icorr;
				logParameter(LogParameter("ibias", QString::number(ibias)+" uA"));
			}
		} else {
			logParameter(LogParameter("vadc3", QString::number(v1)+" V"));
			logParameter(LogParameter("vadc4", QString::number(v2)+" V"));
		}
	}
}

void Daemon::onUBXReceivedTimeTM2(timespec rising, timespec falling, uint32_t accEst, bool valid, uint8_t timeBase, bool utcAvailable)
{
	long double dts=(falling.tv_sec-rising.tv_sec)*1e9;
	dts+=(falling.tv_nsec-rising.tv_nsec);
	if (dts>0.) {
		checkRescaleHisto(ubxTimeLengthHisto, dts);
		ubxTimeLengthHisto.fill(dts);
	}
	long double interval=(rising.tv_sec-lastTimestamp.tv_sec)*1e9;
	interval+=(rising.tv_nsec-lastTimestamp.tv_nsec);
	ubxTimeIntervalHisto.fill(1e-6*interval);
	lastTimestamp=rising;
}
