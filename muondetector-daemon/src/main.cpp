#include <stdio.h>
#include <signal.h>

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QObject>
#include <QHostAddress>
#include <QDir>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <libconfig.h++>

/*
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
*/

#include <custom_io_operators.h>
#include <daemon.h>

static const char* CONFIG_FILE = "/etc/muondetector.conf";

using namespace std;

int getch() {
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}

string getpass(const char *prompt, bool show_asterisk)
{
  const char BACKSPACE=127;
  const char RETURN=10;

  string password;
  unsigned char ch=0;
  ch=getch();
  cout <<prompt<<endl;
  while((ch=getch())!=RETURN)
    {
       if(ch==BACKSPACE)
         {
            if(password.length()!=0)
              {
                 if(show_asterisk)
                 cout <<"\b \b";
                 password.resize(password.length()-1);
              }
         }
       else
         {
             password+=ch;
             if(show_asterisk)
                 cout <<'*';
         }
    }
  cout << endl;
  return password;
}

int main(int argc, char *argv[])
{
    qRegisterMetaType<TcpMessage>("TcpMessage");
	QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("muondetector-daemon");
    QCoreApplication::setApplicationVersion("1.1.2");

	// config file handling
	libconfig::Config cfg;
	
	// Read the file. If there is an error, report it and exit.
	try
	{
		cfg.readFile(CONFIG_FILE);
	}
	catch(const libconfig::FileIOException &fioex)
	{
		std::cerr << "Error while reading config file " << std::string(CONFIG_FILE) << std::endl;
		//return(EXIT_FAILURE);
	}
	catch(const libconfig::ParseException &pex)
	{
		std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
              << " - " << pex.getError() << std::endl;
		return(EXIT_FAILURE);
	}
	
    // command line input management
	QCommandLineParser parser;
	parser.setApplicationDescription("MuonPi cosmic shower muon detector control and configuration program (daemon)\n"
		"with added tcp implementation for synchronisation of "
		"data with a central server");
	parser.addHelpOption();
	parser.addVersionOption();

	// add module path for example /dev/gps0 or /dev/ttyAMA0
	parser.addPositionalArgument("device", QCoreApplication::translate("main", "Path to gps device\n"
		"for example: /dev/ttyAMA0"));

    // login option
    QCommandLineOption mqttLoginOption(QStringList() << "l" << "login",
                                          QCoreApplication::translate("main", "ask for login to the online mqtt server"));
    parser.addOption(mqttLoginOption);

    // station ID (some name for individual stations if someone has multiple)
    QCommandLineOption stationIdOption("id",
        QCoreApplication::translate("main", "Set station ID"),
        QCoreApplication::translate("main", "ID"));
    parser.addOption(stationIdOption);


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
	QCommandLineOption showoutOption(QStringList() << "showoutgoing" << "showout",
		QCoreApplication::translate("main", "show outgoing ubx messages as hex"));
	parser.addOption(showoutOption);

	// show incoming ubx messages as hex
	QCommandLineOption showinOption(QStringList() << "showincoming" << "showin",
		QCoreApplication::translate("main", "show incoming ubx messages as hex"));
	parser.addOption(showinOption);

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

	// daemonAddress option
	QCommandLineOption daemonIpOption(QStringList() << "server" << "daemonAddress",
		QCoreApplication::translate("main", "set gui server ip address"),
		QCoreApplication::translate("main", "daemonAddress"));
	parser.addOption(daemonIpOption);

	// daemonPort option
	QCommandLineOption daemonPortOption(QStringList() << "dp" << "daemonPort",
		QCoreApplication::translate("main", "set gui server port"),
		QCoreApplication::translate("main", "daemonPort"));
	parser.addOption(daemonPortOption);

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
		QCoreApplication::translate("main", "set input signal for ublox interrupt pin:"
			"\n0 - coincidence (AND)"
			"\n1 - anti-coincidence (XOR)"
			"\n2 - discr 1"
			"\n3 - discr 2"),
		QCoreApplication::translate("main", "channel"));
	parser.addOption(pcaChannelOption);

	// biasVoltage for SciPM
	QCommandLineOption biasVoltageOption(QStringList() << "bias" << "vout",
		QCoreApplication::translate("main", "set voltage for SiPM"),
		QCoreApplication::translate("main", "bias voltage"));
	parser.addOption(biasVoltageOption);

	// biasVoltage on or off
	QCommandLineOption biasPowerOnOff(QStringList() << "p",
		QCoreApplication::translate("main", "bias voltage on or off?"));
	parser.addOption(biasPowerOnOff);

    // preamps on:
    QCommandLineOption preamp1Option(QStringList() << "pre1" << "preamp1",
        QCoreApplication::translate("main", "pre-amplifier 1 on or off"));
    parser.addOption(preamp1Option);
    QCommandLineOption preamp2Option(QStringList() << "pre2" << "preamp2",
        QCoreApplication::translate("main", "pre-amplifier 2 on or off"));
    parser.addOption(preamp2Option);

    // gain:
    QCommandLineOption gainOption(QStringList() << "g" << "gain",
        QCoreApplication::translate("main", "gain high"));
    parser.addOption(gainOption);

	// biasVoltage on or off
	QCommandLineOption eventInputOption(QStringList() << "t" << "trigger",
		QCoreApplication::translate("main", "event (trigger) signal input:"
			"\n0 - coincidence (AND)"
			"\n1 - anti-coincidence (XOR)"),
			QCoreApplication::translate("main", "trigger"));
	parser.addOption(eventInputOption);
	

	// process the actual command line arguments given by the user
	parser.process(a);
	const QStringList args = parser.positionalArguments();
	if (args.size() > 1) { cout << "you set additional positional arguments but the program does not use them" << endl; }

    bool ok;
	int verbose = 0;
	if (parser.isSet(verbosityOption)) {
		verbose = parser.value(verbosityOption).toInt(&ok);
		if (!ok) {
			verbose = 0;
			cout << "wrong input verbosity level" << endl;
		}
	}

	// setup all variables for ublox module manager, then make the object run
	QString gpsdevname="";
	if (!args.empty() && args.at(0) != "") {
		gpsdevname = args.at(0);
	} else 
	try
	{
		std::string gpsdevnameCfg = cfg.lookup("ublox_device");
		if (verbose>2) cout << "ublox device: " << gpsdevnameCfg << endl;
		gpsdevname = QString::fromStdString(gpsdevnameCfg);
	}
	catch(const libconfig::SettingNotFoundException &nfex)
	{
		if (verbose>2)
			cerr << "No 'ublox_device' setting in configuration file. Will guess..." << endl;
        QDir directory("/dev","*",QDir::Name, QDir::System);
        QStringList serialports = directory.entryList(QStringList({"ttyS0","ttyAMA0"}));
        if (!serialports.empty()){
            gpsdevname=QString("/dev/"+serialports.at(0));
			if (verbose>2)
				cout << "detected " << gpsdevname << " as most probable candidate" << endl;
        }else{
            cout << "no device selected, will not connect to gps module" << endl;
        }
	}
	
	if (verbose > 4) {
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
	} else
	try
	{
		int baudrateCfg = cfg.lookup("ublox_baud");
		if (verbose>2) cout << "ublox baudrate: " << baudrateCfg << endl;
		baudrate = baudrateCfg;
	}
	catch(const libconfig::SettingNotFoundException &nfex)
	{
		if (verbose>2)
			cerr << "No 'ublox_baud' setting in configuration file. Assuming " << baudrate << endl;
	}
	
	bool showGnssConfig = false;
	showGnssConfig = parser.isSet(showGnssConfigOption);
	quint16 peerPort = 0;
	if (parser.isSet(peerPortOption)) {
		peerPort = parser.value(peerPortOption).toUInt(&ok);
		if (!ok) {
			peerPort = 0;
			cout << "wrong input peerPort (maybe not an integer)" << endl;
		}
	}
	QString peerAddress = "";
	if (parser.isSet(peerIpOption)) {
		peerAddress = parser.value(peerIpOption);
		if (!QHostAddress(peerAddress).toIPv4Address()) {
			if (peerAddress != "localhost" && peerAddress != "local") {
				peerAddress = "";
				cout << "wrong input ipAddress, not an ipv4address" << endl;
			}
		}
	}
	quint16 daemonPort = 0;
	if (parser.isSet(daemonPortOption)) {
		daemonPort = parser.value(daemonPortOption).toUInt(&ok);
		if (!ok) {
			peerPort = 0;
			cout << "wrong input peerPort (maybe not an integer)" << endl;
		}
	}
	QString daemonAddress = "";
	if (parser.isSet(daemonIpOption)) {
		daemonAddress = parser.value(daemonIpOption);
		if (!QHostAddress(daemonAddress).toIPv4Address()) {
			if (daemonAddress != "localhost" && daemonAddress != "local") {
				daemonAddress = "";
				cout << "wrong input ipAddress, not an ipv4address" << endl;
			}
		}
    }
	
	quint8 pcaChannel = 0;
	if (parser.isSet(pcaChannelOption)) {
		pcaChannel = parser.value(pcaChannelOption).toUInt(&ok);
		if (!ok) {
			pcaChannel = 0;
			cout << "wrong input pcaChannel (maybe not an unsigned integer)" << endl;
		}
	} else
	try
	{
		int pcaChannelCfg = cfg.lookup("timing_input");
		if (verbose>2) cout << "timing input: " << pcaChannelCfg << endl;
		pcaChannel = pcaChannelCfg;
	}
	catch(const libconfig::SettingNotFoundException &nfex)
	{
		if (verbose>2)
			cerr << "No 'timing_input' setting in configuration file. Assuming " << (int)pcaChannel << endl;
	}
	
	bool showout = false;
	showout = parser.isSet(showoutOption);
	bool showin = false;
	showin = parser.isSet(showinOption);
	
	float dacThresh[2];
	dacThresh[0] = -1.;
	if (parser.isSet(discr1Option)) {
		dacThresh[0] = parser.value(discr1Option).toFloat(&ok);
		if (!ok) {
			dacThresh[0] = -1.;
			cout << "wrong input discr1 (maybe not a float)" << endl;
		}
	}
	dacThresh[1] = -1.;
	if (parser.isSet(discr2Option)) {
		dacThresh[1] = parser.value(discr2Option).toFloat(&ok);
		if (!ok) {
			dacThresh[1] = -1.;
			cout << "wrong input discr2 (maybe not a float)" << endl;
		}
	}
	float biasVoltage = -1.;
	if (parser.isSet(biasVoltageOption)) {
		biasVoltage = parser.value(biasVoltageOption).toFloat(&ok);
		if (!ok) {
			biasVoltage = -1.;
			cout << "wrong input biasVoltage (maybe not a float)" << endl;
		}
	}
	bool biasPower = false;
	if (parser.isSet(biasPowerOnOff)) {
		biasPower = true;
	} else
	try
	{
		int biasPowerCfg = cfg.lookup("bias_switch");
		if (verbose>2) cout << "bias switch: " << biasPowerCfg << endl;
		biasPower = biasPowerCfg;
	}
	catch(const libconfig::SettingNotFoundException &nfex)
	{
		if (verbose>2)
			cerr << "No 'bias_switch' setting in configuration file. Assuming " << (int)biasPower << endl;
	}

    bool preamp1 = false;
    if (parser.isSet(preamp1Option)) {
        preamp1 = true;
    } else
	try
	{
		int preamp1Cfg = cfg.lookup("preamp1_switch");
		if (verbose>2) cout << "preamp1 switch: " << preamp1Cfg << endl;
		preamp1 = preamp1Cfg;
	}
	catch(const libconfig::SettingNotFoundException &nfex)
	{
		if (verbose>2)
			cerr << "No 'preamp1_switch' setting in configuration file. Assuming " << (int)preamp1 << endl;
	}
	
    bool preamp2 = false;
    if (parser.isSet(preamp2Option)) {
        preamp2 = true;
    } else
	try
	{
		int preamp2Cfg = cfg.lookup("preamp2_switch");
		if (verbose>2) cout << "preamp2 switch: " << preamp2Cfg << endl;
		preamp2 = preamp2Cfg;
	}
	catch(const libconfig::SettingNotFoundException &nfex)
	{
		if (verbose>2)
			cerr << "No 'preamp2_switch' setting in configuration file. Assuming " << (int)preamp2 << endl;
	}
    
    
    bool gain = false;
    if (parser.isSet(gainOption)) {
        gain = true;
    } else {
		try
		{
			int gainCfg = cfg.lookup("gain_switch");
			if (verbose>2) cout << "gain switch: " << gainCfg << endl;
			gain = gainCfg;
		}
		catch(const libconfig::SettingNotFoundException &nfex)
		{
			if (verbose>2)
				cerr << "No 'gain_switch' setting in configuration file. Assuming " << (int)gain << endl;
		}
	}
	
	unsigned int eventSignal = EVT_XOR;
	if (parser.isSet(eventInputOption)) {
		eventSignal = parser.value(eventInputOption).toUInt(&ok);
		if (!ok || eventSignal>1) {
			cerr << "wrong trigger input signal (valid: 0,1)" << endl;
			return -1;
		} else {
			switch (eventSignal) {
				case 1:	eventSignal=EVT_AND;
					break;
				case 0: 	
				default:
					eventSignal=EVT_XOR;
					break;
			}
		}
	} else
	try
	{
		int eventSignalCfg = cfg.lookup("trigger_input");
		if (verbose>2) cout << "event trigger : " << eventSignalCfg << endl;
		eventSignal = eventSignalCfg;
	}
	catch(const libconfig::SettingNotFoundException &nfex)
	{
		if (verbose>2)
			cerr << "No 'trigger_input' setting in configuration file. Assuming " << (int)eventSignal << endl;
	}


    std::string username="";
    std::string password="";
    if (parser.isSet(mqttLoginOption)){
        cout << "To set the login for the mqtt-server, please enter user name:"<<endl;
        cin >> username;
        password = getpass("please enter password:",true);
    } else
	try
	{
		std::string userNameCfg = cfg.lookup("mqtt_user");
		std::string passwordCfg = cfg.lookup("mqtt_password");
		if (verbose) cout << "mqtt user: " << userNameCfg << " passw: " << passwordCfg << endl;
		username=userNameCfg;
		password=passwordCfg;
	}
	catch(const libconfig::SettingNotFoundException &nfex)
	{
		if (verbose)
		cerr << "No 'mqtt_user' or 'mqtt_password' setting in configuration file. Assuming user=''" << endl;
	}
	
    QString stationID = "0";
    if (parser.isSet(stationIdOption)){
        stationID = parser.value(stationIdOption);
    } else
	// Get the station id from config, if it exists
	try
	{
		std::string stationIdString = cfg.lookup("stationID");
		if (verbose) cout << "station id: " << stationIdString << endl;
		stationID = QString::fromStdString(stationIdString);
	}
	catch(const libconfig::SettingNotFoundException &nfex)
	{
		if (verbose)
		cerr << "No 'stationID' setting in configuration file. Assuming stationID='0'" << endl;
	}
	
    /*
    pid_t pid;

    // Fork off the parent process
    pid = fork();

    // An error occurred
    if (pid < 0)
        exit(EXIT_FAILURE);

    // Success: Let the parent terminate
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // On success: The child process becomes session leader
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    // Catch, ignore and handle signals
    //TODO: Implement a working signal handler
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    // Fork off for the second time
    pid = fork();

    // An error occurred
    if (pid < 0)
        exit(EXIT_FAILURE);

    // Success: Let the parent terminate
    if (pid > 0)
        exit(EXIT_SUCCESS);

    // Set new file permissions
    umask(0);

    // Change the working directory to the root directory
    // or another appropriated directory
    chdir("/");

    // Close all open file descriptors
    int x;
    for (x = sysconf(_SC_OPEN_MAX); x>=0; x--)
    {
        close (x);
    }

    // Open the log file
    openlog ("muondetector-daemon", LOG_PID, LOG_DAEMON);
    */
    Daemon daemon(QString::fromStdString(username), QString::fromStdString(password), gpsdevname, verbose, pcaChannel, dacThresh, biasVoltage, biasPower, dumpRaw,
        baudrate, showGnssConfig, eventSignal, peerAddress, peerPort, daemonAddress, daemonPort, showout, showin, preamp1, preamp2, gain, stationID);
	
	return a.exec();
}
