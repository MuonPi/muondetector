#include <stdio.h>
#include <signal.h>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QObject>
#include <QHostAddress>

#include <custom_io_operators.h>
#include <demon.h>

using namespace std;

/* Signal Handler for SIGINT */
void sigintHandler(int sig_num)
{
    /* Reset handler to catch SIGINT next time.
       Refer http://en.cppreference.com/w/c/program/signal */
    printf("killing process %d\n",getpid());
//    signal(SIGINT, SIG_IGN);
//    printf("\n terminated using Ctrl+C \n");
    fflush(stdout);
    exit(0);
}

int main(int argc, char *argv[])
{
	/* Set the SIGINT (Ctrl-C) signal handler to sigintHandler 
       Refer http://en.cppreference.com/w/c/program/signal */
    signal(SIGINT, sigintHandler);
	signal (SIGQUIT, sigintHandler);
	
	QCoreApplication a(argc, argv);
	QCoreApplication::setApplicationName("myon_detector");
    QCoreApplication::setApplicationVersion("1.0");
    //setup_unix_signal_handlers();
    //unix_sig_handler_daemon *d = new unix_sig_handler_daemon();
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<std::string>("std::string");

	// command line input management
	QCommandLineParser parser;
	parser.setApplicationDescription("U-Blox GPS polling and configuration program\n"
        "with added tcp implementation for synchronising "
		"data with a central server");
	parser.addHelpOption();
	parser.addVersionOption();

	// add module path for example /dev/gps0 or /dev/ttyAMA0
	parser.addPositionalArgument("device", QCoreApplication::translate("main", "Path to gps device\n"
		"for example: /dev/ttyAMA0"));

	// verbosity option
	QCommandLineOption verbosityOption(QStringList() << "e" << "verbose",
		QCoreApplication::translate("main", "set verbosity level\n"
			"3 is max"),
		QCoreApplication::translate("main", "verbosity"));
    parser.addOption(verbosityOption);

    // dumpraw option
    QCommandLineOption dumpRawOption("d",
        QCoreApplication::translate("main", "dump raw gps device (NMEA) output to stdout"));
    parser.addOption(dumpRawOption);

    // show GNSS configs
    QCommandLineOption showGnssConfigOption("c",
        QCoreApplication::translate("main", "configure standard ubx protocol messages at start"));
    parser.addOption(showGnssConfigOption);

    // show outgoing ubx messages as hex
    QCommandLineOption showoutOption(QStringList() << "showoutput" << "showout",
        QCoreApplication::translate("main", "show outgoing ubx messages as hex"));
    parser.addOption(showoutOption);

    // peerAddress option
    QCommandLineOption peerIpOption(QStringList() << "peer" << "peerAddress",
        QCoreApplication::translate("main", "set file server ip address"),
        QCoreApplication::translate("main", "peerAddress"));
    parser.addOption(peerIpOption);

    // peerPort option
    QCommandLineOption peerPortOption(QStringList() << "pp" << "peerPort",
        QCoreApplication::translate("main", "set file server port"),
        QCoreApplication::translate("main", "peerPort"));
    parser.addOption(peerPortOption);

    // demonAddress option
    QCommandLineOption demonIpOption(QStringList() << "server" << "demonAddress",
                                      QCoreApplication::translate("main", "set gui server ip address"),
                                      QCoreApplication::translate("main", "demonAddress"));
    parser.addOption(demonIpOption);

    // demonPort option
    QCommandLineOption demonPortOption(QStringList() << "dp" << "demonPort",
                                      QCoreApplication::translate("main", "set gui server port"),
                                      QCoreApplication::translate("main", "demonPort"));
    parser.addOption(demonPortOption);

	// baudrate option
	QCommandLineOption baudrateOption("b",
		QCoreApplication::translate("main", "set baudrate for serial connection"),
		QCoreApplication::translate("main", "baudrate"));
	parser.addOption(baudrateOption);

    // discriminator thresholds:
    QCommandLineOption discr1Option(QStringList() << "discr1" << "thresh1" << "th1",
                                    QCoreApplication::translate("main",
                                                                "set discriminator 1 threshold in Volts"),
                                    QCoreApplication::translate("main", "threshold1"));
    parser.addOption(discr1Option);
    QCommandLineOption discr2Option(QStringList() << "discr2" << "thresh2" << "th2",
                                    QCoreApplication::translate("main",
                                                                "set discriminator 2 threshold in Volts"),
                                    QCoreApplication::translate("main", "threshold2"));
    parser.addOption(discr2Option);

    // pcaChannel to select signal to ublox
    QCommandLineOption pcaChannelOption(QStringList() << "pca" << "signal",
                                        QCoreApplication::translate("main","set signal for ublox:"
                                                                           "0 - coincidence"
                                                                           "1 - XOR"
                                                                           "2 - discr 1"
                                                                           "3 - discr 2"),
                                        QCoreApplication::translate("main", "channel"));
    parser.addOption(pcaChannelOption);

    // biasVoltage for SciPM
    QCommandLineOption biasVoltageOption(QStringList() << "bias" << "vout",
                                         QCoreApplication::translate("main","set voltage for SciPM"),
                                         QCoreApplication::translate("main", "bias voltage"));
    parser.addOption(biasVoltageOption);

    // biasVoltage on or off
    QCommandLineOption biasPowerOnOff(QStringList() << "p",
                                         QCoreApplication::translate("main", "bias voltage on or off?"));
    parser.addOption(biasPowerOnOff);

	// process the actual command line arguments given by the user
	parser.process(a);
	const QStringList args = parser.positionalArguments();
    if (args.size() > 1) { cout << "you set additional positional arguments but the program does not use them" << endl; }


	// setup all variables for ublox module manager, then make the object run
	QString gpsdevname;
	if (!args.empty() && args.at(0) != "") {
		gpsdevname = args.at(0);
	}
	else {
        cout << "no device selected, will not connect to gps module" << endl;
    }
	bool ok;
	int verbose = 0;
	if (parser.isSet(verbosityOption)) {
		verbose = parser.value(verbosityOption).toInt(&ok);
		if (!ok) {
			verbose = 0;
			cout << "wrong input verbosity level" << endl;
		}
	}
    if (verbose > 4){
        cout << "int main running in thread "
             << QString("0x%1").arg((int)QCoreApplication::instance()->thread()) << endl;
    }
    bool dumpRaw = parser.isSet(dumpRawOption);
	int baudrate = 9600;
	if (parser.isSet(baudrateOption)) {
		baudrate = parser.value(baudrateOption).toInt(&ok);
		if (!ok || baudrate < 0) {
			baudrate = 9600;
			cout << "wrong input for baudrate using default " << baudrate << endl;
		}
    }
	bool showGnssConfig = false;
	showGnssConfig = parser.isSet(showGnssConfigOption);
    quint16 peerPort = 0;
    if (parser.isSet(peerPortOption)){
        peerPort = parser.value(peerPortOption).toUInt(&ok);
        if (!ok) {
            peerPort = 0;
            cout << "wrong input peerPort (maybe not an integer)" << endl;
        }
    }
    QString peerAddress = "";
    if (parser.isSet(peerIpOption)){
        peerAddress = parser.value(peerIpOption);
        if (!QHostAddress(peerAddress).toIPv4Address()){
            if (peerAddress != "localhost" && peerAddress != "local"){
                peerAddress = "";
                cout << "wrong input ipAddress, not an ipv4address" << endl;
            }
        }
    }
    quint16 demonPort = 0;
    if (parser.isSet(demonPortOption)){
        demonPort = parser.value(demonPortOption).toUInt(&ok);
        if (!ok) {
            peerPort = 0;
            cout << "wrong input peerPort (maybe not an integer)" << endl;
        }
    }
    QString demonAddress = "";
    if (parser.isSet(demonIpOption)){
        demonAddress = parser.value(demonIpOption);
        if (!QHostAddress(demonAddress).toIPv4Address()){
            if (demonAddress != "localhost" && demonAddress != "local"){
                demonAddress = "";
                cout << "wrong input ipAddress, not an ipv4address" << endl;
            }
        }
    }
    quint8 pcaChannel=0;
    if (parser.isSet(pcaChannelOption)){
        pcaChannel = parser.value(pcaChannelOption).toUInt(&ok);
        if(!ok){
            pcaChannel = 0;
            cout << "wrong input pcaChannel (maybe not an unsigned integer)" << endl;
        }
    }
    bool showout = false;
    showout = parser.isSet(showoutOption);
    float dacThresh[2];
    dacThresh[0] = 0;
    if (parser.isSet(discr1Option)){
        dacThresh[0] = parser.value(discr1Option).toFloat(&ok);
        if (!ok){
            dacThresh[0] = 0;
            cout << "wrong input discr1 (maybe not a float)"<<endl;
        }
    }
    dacThresh[1]=0;
    if (parser.isSet(discr2Option)){
        dacThresh[1] = parser.value(discr2Option).toFloat(&ok);
        if (!ok){
            dacThresh[1] = 0;
            cout << "wrong input discr2 (maybe not a float)"<<endl;
        }
    }
    float biasVoltage = -1;
    if (parser.isSet(biasVoltageOption)){
        biasVoltage = parser.value(biasVoltageOption).toFloat(&ok);
        if (!ok){
            biasVoltage = -1;
            cout << "wrong input biasVoltage (maybe not a float)"<<endl;
        }
    }
    bool biasPower = false;
    if (parser.isSet(biasPowerOnOff)){
        biasPower = true;
    }
    Demon demon(gpsdevname, verbose, pcaChannel, dacThresh, biasVoltage, biasPower, dumpRaw,
        baudrate, showGnssConfig, peerAddress, peerPort, demonAddress, demonPort, showout);
    return a.exec();
}
