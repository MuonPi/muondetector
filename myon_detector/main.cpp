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

    // ip option
    QCommandLineOption ipOption(QStringList() << "ip" << "address",
        QCoreApplication::translate("main", "set server ip address"),
        QCoreApplication::translate("main", "ipAddress"));
    parser.addOption(ipOption);

    // port option
    QCommandLineOption portOption(QStringList() << "p" << "port",
        QCoreApplication::translate("main", "set server port"),
        QCoreApplication::translate("main", "port"));
    parser.addOption(portOption);


	// baudrate option
	QCommandLineOption baudrateOption("b",
		QCoreApplication::translate("main", "set baudrate for serial connection"),
		QCoreApplication::translate("main", "baudrate"));
	parser.addOption(baudrateOption);

    // set number of read cycles
    QCommandLineOption numberReadCyclesOption("n",
        QCoreApplication::translate("main", "number of read cycles"),
        QCoreApplication::translate("main", "number"));
    parser.addOption(numberReadCyclesOption);

    // set timing option
    QCommandLineOption timingOption("t",
        QCoreApplication::translate("main", "cmd=1 for nav-clock\n"
            "cmd=2 for time-tp"),
        QCoreApplication::translate("main", "cmd"));
    parser.addOption(timingOption);

	// dumpraw option
	QCommandLineOption dumpRawOption("d",
		QCoreApplication::translate("main", "dump raw gps device output to stdout"));
	parser.addOption(dumpRawOption);

	// list sats
	QCommandLineOption listSatsOption("L",
		QCoreApplication::translate("main", "show list of satellites in range"));
	parser.addOption(listSatsOption);

	// all sats
	QCommandLineOption allSatsOption("l",
		QCoreApplication::translate("main", "show list of currently available satellites"));
	parser.addOption(allSatsOption);

	// poll
	QCommandLineOption pollOption("p",
		QCoreApplication::translate("main", "poll"));
	parser.addOption(pollOption);


	// show GNSS configs
	QCommandLineOption showGnssConfigOption("c",
		QCoreApplication::translate("main", "show GNSS configs"));
	parser.addOption(showGnssConfigOption);

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
	long int N = -1;
	if (parser.isSet(numberReadCyclesOption)) {
		N = parser.value(numberReadCyclesOption).toInt(&ok);
		if (!ok) {
			N = -1;
			cout << "wrong input for number of read cycles" << endl;
		}
	}
	bool allSats = false;
	allSats = parser.isSet(allSatsOption);
	bool listSats = false;
	listSats = parser.isSet(listSatsOption);
	bool dumpRaw = false;
	dumpRaw = parser.isSet(dumpRawOption);
	int baudrate = 9600;
	if (parser.isSet(baudrateOption)) {
		baudrate = parser.value(baudrateOption).toInt(&ok);
		if (!ok || baudrate < 0) {
			baudrate = 9600;
			cout << "wrong input for baudrate using default " << baudrate << endl;
		}
	}
	bool poll = false;
	poll = parser.isSet(pollOption);
	bool showGnssConfig = false;
	showGnssConfig = parser.isSet(showGnssConfigOption);
	int timingCmd = 1;
	if (parser.isSet(timingOption)) {
		timingCmd = parser.value(timingOption).toInt(&ok);
		if (!ok || timingCmd > 2 || timingCmd < 1) {
			timingCmd = 1;
			cout << "wrong input for timing" << endl;
		}
	}
    quint16 port = 0;
    if (parser.isSet(portOption)){
        port = parser.value(portOption).toUInt(&ok);
        if (!ok) {
            port = 0;
            cout << "wrong input port (maybe not an integer)" << endl;
        }
    }
    QString ipAddress = "";
    if (parser.isSet(ipOption)){
        ipAddress = parser.value(ipOption);
        if (!QHostAddress(ipAddress).toIPv4Address()){
            if (ipAddress != "localhost" && ipAddress != "local"){
                ipAddress = "";
                cout << "wrong input ipAddress, not an ipv4address" << endl;
            }
        }
    }
    bool showout = false;
    showout = parser.isSet(showoutOption);

    Demon demon(gpsdevname, verbose, allSats, listSats, dumpRaw,
        baudrate, poll, showGnssConfig, timingCmd, N, ipAddress, port, showout);

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
