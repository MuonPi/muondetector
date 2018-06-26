#include <QtNetwork>
#include <chrono>
#include <QThread>
#include <QNetworkInterface>
#include <demon.h>

// define pins on the raspberry pi, UBIAS_EN is the power on/off pin for bias voltage
// PREAMP_1/2 enables the DC voltage to power the preamp through the signal cable
#define UBIAS_EN 7
#define PREAMP_1 28
#define PREAMP_2 29

using namespace std;

Demon::Demon(QString new_gpsdevname, int new_verbose, quint8 new_pcaChannel,
    float* new_dacThresh, float new_biasVoltage, bool new_dumpRaw, int new_baudrate,
    bool new_configGnss, QString new_peerAddress, quint16 new_peerPort,
    QString new_demonAddress, quint16 new_demonPort, bool new_showout, QObject *parent)
    : QTcpServer(parent)
{
    // set all variables

    // general
    verbose = new_verbose;
    if (verbose > 4){
        cout << "demon running in thread " << QString("0x%1").arg((int)this->thread()) << endl;
    }

    // for i2c devices
    lm75 = new LM75();
    adc = new ADS1115();
    dac = new MCP4728();
    float *tempThresh = new_dacThresh;
    dacThresh.push_back(tempThresh[0]);
    dacThresh.push_back(tempThresh[1]);
    biasVoltage = new_biasVoltage;
    pca = new PCA9536();
    pcaChannel = new_pcaChannel;
    for (int i = 0; i<2; i++){
        if (dacThresh[i]>=0){
            dac->setVoltage(i,dacThresh[i]);
        }
    }
    if (biasVoltage>=0){
        dac->setVoltage(2,biasVoltage);
    }

    // for gps module
    gpsdevname = new_gpsdevname;
    dumpRaw = new_dumpRaw;
    baudrate = new_baudrate;
    configGnss = new_configGnss;
    showout = new_showout;

    // for separate raspi pin output states
    wiringPiSetup();
    pinMode(UBIAS_EN, 1);
    biasPowerOn = true; // test only
    if (biasPowerOn){
        digitalWrite(UBIAS_EN, 1);
    }else{
        digitalWrite(UBIAS_EN, 0);
    }
    pinMode(PREAMP_1, 1);
    digitalWrite(PREAMP_1, 1);
    pinMode(PREAMP_2, 1);
    digitalWrite(PREAMP_2, 1);

    // for tcp connection with fileServer
    peerPort = new_peerPort;
    if (peerPort == 0){
        peerPort = 51508;
    }
    peerAddress = new_peerAddress;
    if (peerAddress.isEmpty()||peerAddress == "local"||peerAddress == "localhost") {
        peerAddress = QHostAddress(QHostAddress::LocalHost).toString();
    }

    // for own server to communicate with gui
    /*demonAddress = QHostAddress(new_demonAddress);
    if (new_demonAddress == "localhost" || new_demonAddress == "local"){
        demonAddress = QHostAddress(QHostAddress::LocalHost);
    }else{
        QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
        // use the first non-localhost IPv4 address
        for (int i = 0; i < ipAddressesList.size(); ++i) {
            if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
                ipAddressesList.at(i).toIPv4Address()) {
                demonAddress = ipAddressesList.at(i);
                break;
            }
        }
    }*/
    //somehow does not work
    if (new_demonAddress.isEmpty()){
        // if not otherwise specified: listen on all available addresses
        demonAddress = QHostAddress(QHostAddress::Any);
        cout << demonAddress.toString()<<endl;
    }


    demonPort = new_demonPort;
    if (demonPort == 0){
        // maybe think about other fall back solution
        demonPort = 51508;
    }
    if (!this->listen(demonAddress,demonPort)) {
        cout << tr("Unable to start the server: %1.\n").arg(this->errorString());
    }else{
        if (verbose > 4){
        cout <<tr("\nThe server is running on\n\nIP: %1\nport: %2\n\n")
                             .arg(demonAddress.toString()).arg(serverPort());
        }
    }
    flush(cout);

    // start tcp connection and gps module connection
    //connectToServer();
    connectToGps();
    delay(1000);
    if(configGnss){
        configGps();
    }
}

void Demon::connectToGps(){
    // before connecting to gps we have to make sure all other programs are closed
    // and serial echo is off
    QProcess prepareSerial;
    QString command = "stty";
    QStringList args = {"-F", "/dev/ttyAMA0", "-echo", "-onlcr"};
    prepareSerial.start(command,args,QIODevice::ReadWrite);
    prepareSerial.waitForFinished();

    // here is where the magic threading happens look closely
    qtGps = new QtSerialUblox(gpsdevname, gpsTimeout, baudrate, dumpRaw, verbose, showout);
    QThread *gpsThread = new QThread();
    qtGps->moveToThread(gpsThread);
    // connect all signals not coming from Demon to gps
    connect(qtGps,&QtSerialUblox::toConsole, this, &Demon::gpsToConsole);
    connect(gpsThread, &QThread::started, qtGps, &QtSerialUblox::makeConnection);
    connect(qtGps, &QtSerialUblox::destroyed, gpsThread, &QThread::deleteLater);
    connect(qtGps, &QtSerialUblox::gpsRestart, this, &Demon::connectToGps);
    // connect all command signals for ublox module here
    connect(this, &Demon::UBXSetCfgPrt, qtGps, &QtSerialUblox::UBXSetCfgPrt);
    connect(this, &Demon::UBXSetCfgMsg, qtGps, &QtSerialUblox::UBXSetCfgMsg);
    connect(this, &Demon::UBXSetCfgRate, qtGps, &QtSerialUblox::UBXSetCfgRate);
    connect(this, &Demon::sendPoll, qtGps, &QtSerialUblox::sendPoll);
    // connect cfgError signal to output, could also create special errorFunction
    connect(qtGps, &QtSerialUblox::UBXCfgError, this, &Demon::toConsole);

    // after thread start there will be a signal emitted which starts the qtGps makeConnection function
    gpsThread->start();
}

void Demon::connectToServer(){
    QThread *tcpThread = new QThread();
    if (tcpConnection!=nullptr){
        delete(tcpConnection);
    }
    tcpConnection = new TcpConnection(peerAddress, peerPort, verbose);
    tcpConnection->moveToThread(tcpThread);
    connect(tcpThread, &QThread::started, tcpConnection, &TcpConnection::makeConnection);
    connect(tcpThread, &QThread::finished, tcpConnection, &TcpConnection::deleteLater);
    connect(tcpThread, &QThread::finished, tcpThread, &QThread::deleteLater);
    connect(this, &Demon::sendFile, tcpConnection, &TcpConnection::sendFile);
    connect(tcpConnection, &TcpConnection::error, this, &Demon::displaySocketError);
    connect(tcpConnection, &TcpConnection::toConsole, this, &Demon::toConsole);
    connect(tcpConnection, &TcpConnection::connectionTimeout, this, &Demon::connectToServer);
    //connect(this, &Demon::sendMsg, tcpConnection, &TcpConnection::sendMsg);
    //connect(this, &Demon::posixTerminate, tcpConnection, &TcpConnection::onPosixTerminate);
    //connect(tcpConnection, &TcpConnection::stoppedConnection, this, &Demon::stoppedConnection);
    tcpThread->start();
}

void Demon::incomingConnection(qintptr socketDescriptor){
    if (verbose > 4){
        cout << "incomingConnection" <<endl;
    }
    QThread *thread = new QThread();
    TcpConnection *tcpConnection = new TcpConnection(socketDescriptor,verbose);
    tcpConnection->moveToThread(thread);
    connect(thread, &QThread::started, tcpConnection, &TcpConnection::receiveConnection);
    connect(thread, &QThread::finished, tcpConnection, &TcpConnection::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    QObject::connect(tcpConnection, &TcpConnection::toConsole, this, &Demon::toConsole);
    connect(this, &Demon::i2CProperties, tcpConnection, &TcpConnection::sendI2CProperties);
    connect(tcpConnection, &TcpConnection::requestI2CProperties, this, &Demon::sendI2CProperties);
    connect(tcpConnection, &TcpConnection::i2CProperties, this, &Demon::setI2CProperties);
    thread->start();
}

void Demon::pcaSelectTimeMeas(uint8_t channel){
    if (channel>3){
        cout << "can there is no channel > 3" << endl;
        return;
    }
    if (!pca){
        return;
    }
    pca->setOutputPorts(channel);
    pca->setOutputState(channel);
}

void Demon::dacSetThreashold(uint8_t channel, float threashold){
    dac->setVoltage(channel, threashold);
}

void Demon::configGps() {
    // set up ubx as only outPortProtocol
    //emit UBXSetCfgPrt(1,1); // enables on UART port (1) only the UBX protocol
    emit UBXSetCfgPrt(1,PROTO_UBX);
    // deactivate all NMEA messages: (port 6 means ALL ports)
    // for acknowledge we have to think about a better solution
    messageConfiguration.insert(MSG_NMEA_DTM,0);
    messageConfiguration.insert(MSG_NMEA_GBQ,0);
    messageConfiguration.insert(MSG_NMEA_GBS,0);
    messageConfiguration.insert(MSG_NMEA_GGA,0);
    messageConfiguration.insert(MSG_NMEA_GLL,0);
    messageConfiguration.insert(MSG_NMEA_GLQ,0);
    messageConfiguration.insert(MSG_NMEA_GNQ,0);
    messageConfiguration.insert(MSG_NMEA_GNS,0);
    messageConfiguration.insert(MSG_NMEA_GPQ,0);
    messageConfiguration.insert(MSG_NMEA_GRS,0);
    messageConfiguration.insert(MSG_NMEA_GSA,0);
    messageConfiguration.insert(MSG_NMEA_GST,0);
    messageConfiguration.insert(MSG_NMEA_GSV,0);
    messageConfiguration.insert(MSG_NMEA_RMC,0);
    messageConfiguration.insert(MSG_NMEA_TXT,0);
    messageConfiguration.insert(MSG_NMEA_VLW,0);
    messageConfiguration.insert(MSG_NMEA_VTG,0);
    messageConfiguration.insert(MSG_NMEA_ZDA,0);
    messageConfiguration.insert(MSG_NMEA_POSITION,0);
    emit UBXSetCfgMsg(MSG_NMEA_DTM,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GBQ,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GBS,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GGA,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GLL,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GLQ,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GNQ,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GNS,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GPQ,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GRS,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GSA,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GST,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_GSV,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_RMC,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_TXT,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_VLW,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_VTG,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_ZDA,6,0);
    emit UBXSetCfgMsg(MSG_NMEA_POSITION,6,0);

    // set protocol configuration for ports
    // messageConfiguration: -1 means unknown, 0 means off, some positive value means update time
    const int measrate = 10;
    messageConfiguration.insert(MSG_CFG_RATE, measrate);
    messageConfiguration.insert(MSG_TIM_TM2, 1);
    messageConfiguration.insert(MSG_TIM_TP, 51);
    messageConfiguration.insert(MSG_NAV_TIMEUTC, 20);
    messageConfiguration.insert(MSG_MON_HW, 47);
    messageConfiguration.insert(MSG_NAV_SAT, 59);
    messageConfiguration.insert(MSG_NAV_TIMEGPS, 61);
    messageConfiguration.insert(MSG_NAV_SOL, 67);
    messageConfiguration.insert(MSG_NAV_STATUS, 71);
    messageConfiguration.insert(MSG_NAV_CLOCK, 89);
    messageConfiguration.insert(MSG_MON_TXBUF, 97);
    messageConfiguration.insert(MSG_NAV_SBAS, 255);
    messageConfiguration.insert(MSG_NAV_DOP, 101);
    messageConfiguration.insert(MSG_NAV_SVINFO, 49);
    emit UBXSetCfgRate(1000 / measrate, 1); // MSG_CFG_RATE
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

    delay(1000);
    emit sendPoll(0x0600,1);
}

void Demon::UBXReceivedAckAckNak(bool ackAck, uint16_t ackedMsgID, uint16_t ackedCfgMsgID){
    // the value was already set correctly,
    // if not acknowledged or timeout we set the value to -1 (unknown/undefined)
    if (!ackAck){
        switch(ackedMsgID){
        case MSG_CFG_MSG:
            messageConfiguration.insert(ackedCfgMsgID,-1);
            break;
        default:
            messageConfiguration.insert(ackedMsgID,-1);
            break;
        }
    }
}


void Demon::gpsPropertyUpdatedGnss(std::vector<GnssSatellite> data,
                                std::chrono::duration<double> lastUpdated){
    //if (listSats){
    if (verbose > 3){
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
        for (unsigned int i = 0; i < sats.size(); i++){
            sats[i].Print(i, false);
        }
    }
}
void Demon::gpsPropertyUpdatedUint8(uint8_t data, std::chrono::duration<double> updateAge,
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

void Demon::gpsPropertyUpdatedUint32(uint32_t data, chrono::duration<double> updateAge,
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

void Demon::gpsPropertyUpdatedInt32(int32_t data, std::chrono::duration<double> updateAge,
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

void Demon::setI2CProperties(I2cProperty i2cProperty, bool setProperties){
    if (!setProperties){
        return;
    }
}

void Demon::toConsole(QString data) {
    cout << data << endl;
}

void Demon::gpsToConsole(QString data){
    cout << data << flush;
}

void Demon::gpsConnectionError(){

}

void Demon::stoppedConnection(QString hostName, quint16 port, quint32 connectionTimeout, quint32 connectionDuration){
    cout << "stopped connection with " << hostName<<":"<<port<<endl;
    cout<<"connection timeout at "<<connectionTimeout<<"  connection lasted "<<connectionDuration<<"s"<<endl;
}

void Demon::displayError(QString message)
{
    cout <<"Demon: "<< message << endl;
}

void Demon::displaySocketError(int socketError, QString message)
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

void Demon::delay(int millisecondsWait)
{
	QEventLoop loop;
	QTimer t;
	t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
	t.start(millisecondsWait);
	loop.exec();
}

void Demon::printTimestamp()
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

void Demon::sendI2CProperties(){
    I2cProperty i2cProperty(pcaChannel, dacThresh.at(0), dacThresh.at(1), biasVoltage, biasPowerOn);
    emit i2CProperties(i2cProperty);
}
