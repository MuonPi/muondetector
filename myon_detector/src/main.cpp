#include <QCoreApplication>
#include <QCommandLineParser>
#include <QObject>
#include <QHostAddress>
//#include <unistd.h>
#include "custom_io_operators.h"
#include "demon.h"
//#include "unix_sig_handler_daemon.h" //for handling unix signals
using namespace std;

/* for handling unix signals, does not really work
static int setup_unix_signal_handlers()
{
    struct sigaction hup, term, interrupt;

    hup.sa_handler = unix_sig_handler_daemon::hupSignalHandler;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = 0;
    hup.sa_flags |= SA_RESTART;

    if (sigaction(SIGHUP, &hup, 0))
       return 1;

    term.sa_handler = unix_sig_handler_daemon::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags |= SA_RESTART;

    if (sigaction(SIGTERM, &term, 0))
       return 2;

    interrupt.sa_handler = unix_sig_handler_daemon::intSignalHandler;
    sigemptyset(&interrupt.sa_mask);
    interrupt.sa_flags |= SA_RESTART;

    if (sigaction(SIGINT, &interrupt, 0))
       return 3;

    return 0;
}
*/

int main(int argc, char *argv[])
{
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

    //

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

	// dumpraw option
	QCommandLineOption dumpRawOption("d",
		QCoreApplication::translate("main", "dump raw gps device output to stdout"));
	parser.addOption(dumpRawOption);

	// show GNSS configs
	QCommandLineOption showGnssConfigOption("c",
		QCoreApplication::translate("main", "show GNSS configs"));
	parser.addOption(showGnssConfigOption);

    // pcaChannel to select signal to ublox
    QCommandLineOption pcaChannelOption(QStringList() << "pca" << "signal",
                                        QCoreApplication::translate("main","set signal for ublox:"
                                                                           "0 - coincidence"
                                                                           "1 - XOR"
                                                                           "2 - discr 1"
                                                                           "3 - discr 2"));
    parser.addOption(pcaChannelOption);


    // show outgoing ubx messages as hex
    QCommandLineOption showoutOption(QStringList() << "showoutput" << "showout",
        QCoreApplication::translate("main", "show outgoing ubx messages as hex"));
    parser.addOption(showoutOption);

	// process the actual command line arguments given by the user
	parser.process(a);
	const QStringList args = parser.positionalArguments();
	if (args.size() > 1) { cout << "you set positional arguments but the program does not use them" << endl; }


	// setup all variables for ublox module manager, then make the object run
	QString gpsdevname;
	if (!args.empty() && args.at(0) != "") {
		gpsdevname = args.at(0);
	}
	else {
		cout << "no device selected" << endl;
		exit(-1);
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
    bool showout = false;
    showout = parser.isSet(showoutOption);
    float dacThresh[2];
    dacThresh[0]=0;
    dacThresh[1]=0;

    Demon demon(gpsdevname, verbose, pcaChannel, dacThresh, dumpRaw,
        baudrate, showGnssConfig, peerAddress, peerPort, demonAddress, demonPort, showout);

    /* handling posix signals does not really work atm
    QObject::connect(d, SIGNAL(myIntSignal()),&Demon,
                     SLOT(onPosixTerminateReceived()));
    QObject::connect(d, SIGNAL(myHupSignal()),&Demon,
                     SLOT(onPosixTerminateReceived()));
    QObject::connect(d, SIGNAL(myTermSignal()),&Demon,
                     SLOT(onPosixTerminateReceived()));
    */
    return a.exec();
}
