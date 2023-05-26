#include <signal.h>
#include <stdio.h>

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QHostAddress>
#include <QObject>
#include <iostream>
#include <libconfig.h++>
#include <termios.h>
#include <unistd.h>

#include "daemon.h"
#include <config.h>
#include <gpio_pin_definitions.h>

static const std::string CONFIG_FILE = std::string(MuonPi::Config::file);
static const std::string SETTINGS_FILE = std::string(MuonPi::Config::data_path) + std::string(MuonPi::Config::persistant_settings_file);
static int verbose = 0;

[[nodiscard]] auto getch() -> int;

auto getch() -> int
{
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

// The application's custom message handler
// All outputs to console should go through the QT message mechanism:
// i.e. qDebug(), qWarning(), qCritical(), qFatal()
void messageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char* file = context.file ? context.file : "";
    const char* function = context.function ? context.function : "";
    switch (type) {
    case QtDebugMsg:
        if (verbose) {
            fprintf(stdout, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
            fflush(stdout);
        }
        break;
    case QtInfoMsg:
        fprintf(stdout, "Info: %s\n", localMsg.constData());
        fflush(stdout);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s\n", localMsg.constData());
        fflush(stderr);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", localMsg.constData());
        fflush(stderr);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", localMsg.constData());
        fflush(stderr);
        break;
    }
}

int main(int argc, char* argv[])
{
    // first, we must set the locale to be independent of the number format of the system's locale.
    // We rely on parsing floating point numbers with a decimal point (not a komma) which might fail if not setting the classic locale
    std::locale::global(std::locale::classic());

    qRegisterMetaType<TcpMessage>("TcpMessage");
    qRegisterMetaType<GnssPosStruct>("GnssPosStruct");
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<int8_t>("int8_t");
    qRegisterMetaType<bool>("bool");
    qRegisterMetaType<CalibStruct>("CalibStruct");
    qRegisterMetaType<std::vector<GnssSatellite>>("std::vector<GnssSatellite>");
    qRegisterMetaType<std::vector<GnssConfigStruct>>("std::vector<GnssConfigStruct>");
    qRegisterMetaType<std::chrono::duration<double>>("std::chrono::duration<double>");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<LogParameter>("LogParameter");
    qRegisterMetaType<UbxTimePulseStruct>("UbxTimePulseStruct");
    qRegisterMetaType<UbxDopStruct>("UbxDopStruct");
    qRegisterMetaType<timespec>("timespec");
    qRegisterMetaType<GPIO_SIGNAL>("GPIO_SIGNAL");
    qRegisterMetaType<GnssMonHwStruct>("GnssMonHwStruct");
    qRegisterMetaType<GnssMonHw2Struct>("GnssMonHw2Struct");
    qRegisterMetaType<UbxTimeMarkStruct>("UbxTimeMarkStruct");
    qRegisterMetaType<I2cDeviceEntry>("I2cDeviceEntry");
    qRegisterMetaType<ADC_SAMPLING_MODE>("ADC_SAMPLING_MODE");
    qRegisterMetaType<MuonPi::Version::Version>("MuonPi::Version::Version");
    qRegisterMetaType<UbxDynamicModel>("UbxDynamicModel");
    qRegisterMetaType<EventTime>("EventTime");
    qRegisterMetaType<PigpiodHandler::EventEdge>("PigpiodHandler::EventEdge");
    
    qInstallMessageHandler(messageOutput);

    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("muondetector-daemon");
    QCoreApplication::setApplicationVersion(QString::fromStdString(MuonPi::Version::software.string()));
    // config file handling
    libconfig::Config cfg;
    libconfig::Config settings;

    qInfo() << "MuonPi Muondetector Daemon "
            << "V" + QString::fromStdString(MuonPi::Version::software.string())
            << "(build " + QString(__TIMESTAMP__) + ")";

    // Read the file. If there is an error, report it and exit.
    try {
        cfg.readFile(CONFIG_FILE.c_str());
    } catch (const libconfig::FileIOException& fioex) {
        qWarning() << "Error while reading config file" << QString::fromStdString(CONFIG_FILE);
    } catch (const libconfig::ParseException& pex) {
        qFatal(qPrintable("Parse error at " + QString(pex.getFile()) + " : line " + QString(pex.getLine()) + " - " + QString(pex.getError())));
        return (EXIT_FAILURE);
    }

    // Read in the settings file. If there is an error, create the settings
    // tree and proceed anyway
    try {
        settings.readFile(SETTINGS_FILE.c_str());
    } catch (const libconfig::FileIOException& fioex) {
        // Find the stored settings in settings file. Add all entries if they don't yet
        // exist.
        libconfig::Setting& root = settings.getRoot();
        // create setting fields
        root.add("geo_handling", libconfig::Setting::TypeGroup);
        libconfig::Setting& geo_handling = root["geo_handling"];
        geo_handling.add("mode", libconfig::Setting::TypeString) = "Auto";
        geo_handling.add("static_coordinates", libconfig::Setting::TypeGroup);
        libconfig::Setting& static_coords = geo_handling["static_coordinates"];
        static_coords.add("lon", libconfig::Setting::TypeFloat) = 0.;
        static_coords.add("lat", libconfig::Setting::TypeFloat) = 0.;
        static_coords.add("alt", libconfig::Setting::TypeFloat) = 0.;
        static_coords.add("hor_error", libconfig::Setting::TypeFloat) = 0.;
        static_coords.add("vert_error", libconfig::Setting::TypeFloat) = 0.;
        // Write out the updated configuration.
        try {
            settings.writeFile(SETTINGS_FILE.c_str());
            qInfo() << "Initialized settings successfully written to: " << QString::fromStdString(SETTINGS_FILE);
        } catch (const libconfig::FileIOException& fioex_new) {
            qCritical() << "I/O error while writing settings file: " << QString::fromStdString(SETTINGS_FILE);
        }
    } catch (const libconfig::ParseException& pex) {
        qFatal(qPrintable("Parse error at " + QString(pex.getFile()) + " : line " + QString(pex.getLine()) + " - " + QString(pex.getError())));
        return (EXIT_FAILURE);
    }

    // command line input management
    QCommandLineParser parser;
    parser.setApplicationDescription("MuonPi cosmic shower muon detector control and configuration program (daemon)\n"
                                     "with added tcp implementation for synchronisation of "
                                     "data with a central server through MQTT protocol");
    parser.addHelpOption();
    parser.addVersionOption();

    // add module path for example /dev/gps0 or /dev/ttyAMA0
    parser.addPositionalArgument("device", QCoreApplication::translate("main", "path to u-blox GNSS device\n"
                                                                               "e.g. /dev/ttyAMA0"));

    // station ID (some name for individual stations if someone has multiple)
    QCommandLineOption stationIdOption("id",
        QCoreApplication::translate("main", "set station ID"),
        QCoreApplication::translate("main", "ID"));
    parser.addOption(stationIdOption);

    // verbosity option
    QCommandLineOption verbosityOption(QStringList() << "e"
                                                     << "verbose",
        QCoreApplication::translate("main", "set verbosity level\n"
                                            "5 is max"),
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
    QCommandLineOption showoutOption(QStringList() << "showoutgoing"
                                                   << "showout",
        QCoreApplication::translate("main", "show outgoing ubx messages as hex"));
    parser.addOption(showoutOption);

    // show incoming ubx messages as hex
    QCommandLineOption showinOption(QStringList() << "showincoming"
                                                  << "showin",
        QCoreApplication::translate("main", "show incoming ubx messages as hex"));
    parser.addOption(showinOption);

    // daemonAddress option
    QCommandLineOption daemonIpOption(QStringList() << "server"
                                                    << "daemonAddress",
        QCoreApplication::translate("main", "set gui server ip address"),
        QCoreApplication::translate("main", "daemonAddress"));
    parser.addOption(daemonIpOption);

    // daemonPort option
    QCommandLineOption daemonPortOption(QStringList() << "dp"
                                                      << "daemonPort",
        QCoreApplication::translate("main", "set gui server port"),
        QCoreApplication::translate("main", "daemonPort"));
    parser.addOption(daemonPortOption);

    // baudrate option
    QCommandLineOption baudrateOption("b",
        QCoreApplication::translate("main", "set baudrate for serial connection"),
        QCoreApplication::translate("main", "baudrate"));
    parser.addOption(baudrateOption);

    // discriminator thresholds:
    QCommandLineOption discr1Option(QStringList() << "discr1"
                                                  << "thresh1"
                                                  << "th1",
        QCoreApplication::translate("main",
            "set discriminator 1 threshold in Volts"),
        QCoreApplication::translate("main", "threshold1"));
    parser.addOption(discr1Option);
    QCommandLineOption discr2Option(QStringList() << "discr2"
                                                  << "thresh2"
                                                  << "th2",
        QCoreApplication::translate("main",
            "set discriminator 2 threshold in Volts"),
        QCoreApplication::translate("main", "threshold2"));
    parser.addOption(discr2Option);

    // pcaPortMask to select signal to ublox
    QCommandLineOption pcaPortMaskOption(QStringList() << "pca"
                                                       << "signal",
        QCoreApplication::translate("main", "set input signal for ublox interrupt pin:"
                                            "\n0 - coincidence (AND)"
                                            "\n1 - anti-coincidence (XOR)"
                                            "\n2 - discr 1"
                                            "\n3 - discr 2"
                                            "\n4 - vcc"
                                            "\n5 - timepulse"
                                            "\n6 - N/A"
                                            "\n7 - ext signal"),
        QCoreApplication::translate("main", "channel"));
    parser.addOption(pcaPortMaskOption);

    // biasVoltage for SiPM
    QCommandLineOption biasVoltageOption(QStringList() << "bias"
                                                       << "vout",
        QCoreApplication::translate("main", "set bias voltage for SiPM"),
        QCoreApplication::translate("main", "bias voltage"));
    parser.addOption(biasVoltageOption);

    // biasVoltage on or off
    QCommandLineOption biasPowerOnOff(QStringList() << "p",
        QCoreApplication::translate("main", "bias voltage on or off"));
    parser.addOption(biasPowerOnOff);

    // preamps on:
    QCommandLineOption preamp1Option(QStringList() << "pre1"
                                                   << "preamp1",
        QCoreApplication::translate("main", "preamplifier channel 1 on/off (0/1)"));
    parser.addOption(preamp1Option);
    QCommandLineOption preamp2Option(QStringList() << "pre2"
                                                   << "preamp2",
        QCoreApplication::translate("main", "preamplifier channel 2 on/off (0/1)"));
    parser.addOption(preamp2Option);

    // gain:
    QCommandLineOption gainOption(QStringList() << "g"
                                                << "gain",
        QCoreApplication::translate("main", "peak detector high gain select"));
    parser.addOption(gainOption);

    // event trigger for ADC
    QCommandLineOption eventInputOption(QStringList() << "t"
                                                      << "trigger",
        QCoreApplication::translate("main", "event (trigger) signal input:"
                                            "\n0 - coincidence (AND)"
                                            "\n1 - anti-coincidence (XOR)"),
        QCoreApplication::translate("main", "trigger"));
    parser.addOption(eventInputOption);

    // input polarity switch:
    QCommandLineOption pol1Option(QStringList() << "pol1"
                                                << "polarity1",
        QCoreApplication::translate("main", "input polarity ch1 negative (0) or positive (1)"));
    parser.addOption(pol1Option);
    QCommandLineOption pol2Option(QStringList() << "pol2"
                                                << "polarity2",
        QCoreApplication::translate("main", "input polarity ch2 negative (0) or positive (1)"));
    parser.addOption(pol2Option);

    // process the actual command line arguments given by the user
    parser.process(a);
    const QStringList args = parser.positionalArguments();
    if (args.size() > 1) {
        qWarning() << "you set additional positional arguments but the program does not use them";
    }

    Daemon::configuration daemonConfig;

    bool ok;
    if (parser.isSet(verbosityOption)) {
        verbose = parser.value(verbosityOption).toInt(&ok);
        if (!ok) {
            verbose = 0;
            qWarning() << "wrong verbosity level selection...setting to 0";
        }
    }
    daemonConfig.verbose = verbose;

    try {
        daemonConfig.maxGeohashLength = cfg.lookup("max_geohash_length");
    } catch (const libconfig::SettingNotFoundException&) {
    }

    try {
        daemonConfig.storeLocal = cfg.lookup("store_local");
    } catch (const libconfig::SettingNotFoundException&) {
    }

    try {
        int model = cfg.lookup("gnss_dynamic_model");
        daemonConfig.gnss_dynamic_model = static_cast<UbxDynamicModel>(model);
    } catch (const libconfig::SettingNotFoundException&) {
    }

    // setup all variables for ublox module manager, then make the object run
    if (!args.empty() && args.at(0) != "") {
        daemonConfig.gpsdevname = args.at(0);
    } else
        try {
            std::string gpsdevnameCfg = cfg.lookup("ublox_device");
            if (verbose > 2)
                std::cout << "ublox device: " << gpsdevnameCfg << std::endl;
            daemonConfig.gpsdevname = QString::fromStdString(gpsdevnameCfg);
        } catch (const libconfig::SettingNotFoundException& nfex) {
            qWarning() << "No 'ublox_device' setting in configuration file. Will guess...";
            QDir directory("/dev", "*", QDir::Name, QDir::System);
            QStringList serialports = directory.entryList(QStringList({ "ttyS0", "ttyAMA0", "serial0" }));
            if (!serialports.empty()) {
                daemonConfig.gpsdevname = QString("/dev/" + serialports.last());
                qInfo() << "detected" << daemonConfig.gpsdevname << "as most probable candidate";
            } else {
                qCritical() << "no device selected, will not connect to GNSS module";
            }
        }

    if (verbose > 4) {
        qDebug() << "int main running in thread"
                 << QString("0x%1").arg(reinterpret_cast<std::uint64_t>(QCoreApplication::instance()->thread()));
    }
    daemonConfig.gnss_dump_raw = parser.isSet(dumpRawOption);

    if (parser.isSet(baudrateOption)) {
        daemonConfig.gnss_baudrate = parser.value(baudrateOption).toInt(&ok);
        if (!ok || daemonConfig.gnss_baudrate < 0) {
            daemonConfig.gnss_baudrate = 9600;
            qWarning() << "wrong input for baudrate...using default:" << daemonConfig.gnss_baudrate;
        }
    } else
        try {
            int baudrateCfg = cfg.lookup("ublox_baud");
            if (verbose > 2)
                qInfo() << "ublox baudrate:" << baudrateCfg;
            daemonConfig.gnss_baudrate = baudrateCfg;
        } catch (const libconfig::SettingNotFoundException& nfex) {
            if (verbose > 1)
                qWarning() << "No 'ublox_baud' setting in configuration file. Assuming" << daemonConfig.gnss_baudrate;
        }

    daemonConfig.gnss_config = parser.isSet(showGnssConfigOption);

    try {
        int port = cfg.lookup("tcp_port");
        if (verbose > 2)
            qDebug() << "tcp_port (listen port): " << port;
        daemonConfig.serverPort = static_cast<quint16>(port);
    } catch (const libconfig::SettingNotFoundException&) {
    }
    if (parser.isSet(daemonPortOption)) {
        daemonConfig.serverPort = parser.value(daemonPortOption).toUInt(&ok);
        if (!ok) {
            daemonConfig.serverPort = 0;
            qCritical() << "wrong input peerPort (maybe not an integer)";
        }
    }

    try {
        std::string tcpIpCfg = cfg.lookup("tcp_ip");
        if (verbose > 2)
            qDebug() << "tcp_ip (listen ip): " << QString::fromStdString(tcpIpCfg);
        daemonConfig.serverAddress = QString::fromStdString(tcpIpCfg);
    } catch (const libconfig::SettingNotFoundException&) {
    }
    if (parser.isSet(daemonIpOption)) {
        daemonConfig.serverAddress = parser.value(daemonIpOption);
        if (!QHostAddress(daemonConfig.serverAddress).toIPv4Address()) {
            if (daemonConfig.serverAddress != "localhost" && daemonConfig.serverAddress != "local") {
                daemonConfig.serverAddress = "";
                qCritical() << "wrong daemon ipAddress, not an ipv4address";
            }
        }
    }

    if (parser.isSet(pcaPortMaskOption)) {
        daemonConfig.pcaPortMask = parser.value(pcaPortMaskOption).toUInt(&ok);
        if (!ok) {
            daemonConfig.pcaPortMask = 0;
            qCritical() << "wrong input pcaPortMask (maybe not an unsigned integer)";
        }
    } else
        try {
            int pcaPortMaskCfg = cfg.lookup("timing_input");
            if (verbose > 2)
                qDebug() << "timing input: " << pcaPortMaskCfg;
            daemonConfig.pcaPortMask = pcaPortMaskCfg;
        } catch (const libconfig::SettingNotFoundException& nfex) {
            qWarning() << "No 'timing_input' setting in configuration file. Assuming" << (int)daemonConfig.pcaPortMask;
        }

    daemonConfig.showout = parser.isSet(showoutOption);
    daemonConfig.showin = parser.isSet(showinOption);

    if (parser.isSet(discr1Option)) {
        daemonConfig.thresholdVoltage[0] = parser.value(discr1Option).toFloat(&ok);
        if (!ok) {
            daemonConfig.thresholdVoltage[0] = -1.;
            qCritical() << "error in value for discr1 (maybe not a float)";
        }
    }
    if (parser.isSet(discr2Option)) {
        daemonConfig.thresholdVoltage[1] = parser.value(discr2Option).toFloat(&ok);
        if (!ok) {
            daemonConfig.thresholdVoltage[1] = -1.;
            qCritical() << "error in value for discr2 (maybe not a float)";
        }
    }
    if (parser.isSet(biasVoltageOption)) {
        daemonConfig.biasVoltage = parser.value(biasVoltageOption).toFloat(&ok);
        if (!ok) {
            daemonConfig.biasVoltage = -1.;
            qCritical() << "error in value for biasVoltage (maybe not a float)";
        }
    }

    if (parser.isSet(biasPowerOnOff)) {
        daemonConfig.bias_ON = true;
    } else
        try {
            int biasPowerCfg = cfg.lookup("bias_switch");
            if (verbose > 2)
                qDebug() << "bias switch:" << biasPowerCfg;
            daemonConfig.bias_ON = biasPowerCfg;
        } catch (const libconfig::SettingNotFoundException& nfex) {
            qWarning() << "No 'bias_switch' setting in configuration file. Assuming" << (int)daemonConfig.bias_ON;
        }

    if (parser.isSet(preamp1Option)) {
        daemonConfig.preamp_enable[0] = true;
    } else
        try {
            int preamp1Cfg = cfg.lookup("preamp1_switch");
            if (verbose > 2)
                qDebug() << "preamp1 switch:" << preamp1Cfg;
            daemonConfig.preamp_enable[0] = preamp1Cfg;
        } catch (const libconfig::SettingNotFoundException& nfex) {
            qWarning() << "No 'preamp1_switch' setting in configuration file. Assuming" << (int)daemonConfig.preamp_enable[0];
        }

    if (parser.isSet(preamp2Option)) {
        daemonConfig.preamp_enable[1] = true;
    } else
        try {
            int preamp2Cfg = cfg.lookup("preamp2_switch");
            if (verbose > 2)
                qDebug() << "preamp2 switch:" << preamp2Cfg;
            daemonConfig.preamp_enable[1] = preamp2Cfg;
        } catch (const libconfig::SettingNotFoundException& nfex) {
            qWarning() << "No 'preamp2_switch' setting in configuration file. Assuming " << (int)daemonConfig.preamp_enable[1];
        }

    if (parser.isSet(gainOption)) {
        daemonConfig.hi_gain = true;
    } else {
        try {
            int gainCfg = cfg.lookup("gain_switch");
            if (verbose > 2)
                qDebug() << "gain switch:" << gainCfg;
            daemonConfig.hi_gain = gainCfg;
        } catch (const libconfig::SettingNotFoundException& nfex) {
            if (verbose > 0)
                qWarning() << "No 'gain_switch' setting in configuration file. Assuming" << (int)daemonConfig.hi_gain;
        }
    }

    int eventTriggerCfg { -1 };
    daemonConfig.eventTrigger = EVT_AND;
    if (parser.isSet(eventInputOption)) {
        eventTriggerCfg = parser.value(eventInputOption).toInt(&ok);
        if (!ok || eventTriggerCfg > 3) {
            qCritical() << "wrong trigger input signal (valid: 0..3)";
            return -1;
        }
    } else
        try {
            eventTriggerCfg = cfg.lookup("trigger_input");
            if (verbose > 2)
                qDebug() << "event trigger : " << eventTriggerCfg;
        } catch (const libconfig::SettingNotFoundException& nfex) {
            qWarning() << "No 'trigger_input' setting in configuration file. Assuming signal" << QString::fromStdString(GPIO_SIGNAL_MAP.at(daemonConfig.eventTrigger).name);
        }

    switch (eventTriggerCfg) {
    case 0:
        daemonConfig.eventTrigger = EVT_XOR;
        break;
    case 1:
        daemonConfig.eventTrigger = EVT_AND;
        break;
    case 2:
        daemonConfig.eventTrigger = TIME_MEAS_OUT;
        break;
    case 3:
        daemonConfig.eventTrigger = EXT_TRIGGER;
        break;
    default:
        daemonConfig.eventTrigger = EVT_AND;
        break;
    }

    if (parser.isSet(pol1Option)) {
        unsigned int pol1int = parser.value(pol1Option).toUInt(&ok);
        if (!ok || pol1int > 1) {
            qCritical() << "wrong input polarity setting ch1 (valid: 0,1)";
            return -1;
        } else {
            daemonConfig.polarity[0] = (bool)pol1int;
        }
    } else {
        try {
            int pol1Cfg = cfg.lookup("input1_polarity");
            if (verbose > 2)
                qDebug() << "input polarity ch1:" << pol1Cfg;
            daemonConfig.polarity[0] = (bool)pol1Cfg;
        } catch (const libconfig::SettingNotFoundException& nfex) {
            if (verbose > 0)
                qWarning() << "No 'input1_polarity' setting in configuration file. Assuming" << (int)daemonConfig.polarity[0];
        }
    }

    if (parser.isSet(pol2Option)) {
        unsigned int pol2int = parser.value(pol2Option).toUInt(&ok);
        if (!ok || pol2int > 1) {
            qCritical() << "wrong input polarity setting ch2 (valid: 0,1)";
            return -1;
        } else {
            daemonConfig.polarity[1] = (bool)pol2int;
        }
    } else {
        try {
            int pol2Cfg = cfg.lookup("input2_polarity");
            if (verbose > 2)
                qDebug() << "input polarity ch2:" << pol2Cfg;
            daemonConfig.polarity[1] = (bool)pol2Cfg;
        } catch (const libconfig::SettingNotFoundException& nfex) {
            if (verbose > 0)
                qWarning() << "No 'input2_polarity' setting in configuration file. Assuming" << (int)daemonConfig.polarity[1];
        }
    }

    try {
        std::string userNameCfg = cfg.lookup("mqtt_user");
        std::string passwordCfg = cfg.lookup("mqtt_password");
        if (verbose > 2)
            qDebug() << "mqtt user: " << QString::fromStdString(userNameCfg) << " passw: " << QString::fromStdString(passwordCfg);

        daemonConfig.username = QString::fromStdString(userNameCfg);
        daemonConfig.password = QString::fromStdString(passwordCfg);
    } catch (const libconfig::SettingNotFoundException& nfex) {
        if (verbose > 1)
            qDebug() << "No 'mqtt_user' or 'mqtt_password' setting in configuration file. Will continue with previously stored credentials";
    }

    if (parser.isSet(stationIdOption)) {
        daemonConfig.station_ID = parser.value(stationIdOption);
    } else {
        // Get the station id from config, if it exists
        try {
            std::string stationIdString = cfg.lookup("stationID");
            if (verbose)
                qInfo() << "station id: " << QString::fromStdString(stationIdString);
            daemonConfig.station_ID = QString::fromStdString(stationIdString);
        } catch (const libconfig::SettingNotFoundException& nfex) {
            qWarning() << "No 'stationID' setting in configuration file. Assuming stationID='0'";
        }
    }

    // Find the stored settings in settings file. Add intermediate entries if they don't yet
    // exist.
    libconfig::Setting& root = settings.getRoot();

#define ENUM_CAST static_cast<size_t>

    if (root.exists("geo_handling")) {
        // try to read in the stored geo handling fields
        std::string mode_str = settings.lookup("geo_handling.mode");
        qDebug() << "mode = " << QString::fromStdString(mode_str);
        if (mode_str == PositionModeConfig::mode_name[ENUM_CAST(PositionModeConfig::Mode::Static)]) {
            daemonConfig.position_mode_config.mode = PositionModeConfig::Mode::Static;
        } else if (mode_str == PositionModeConfig::mode_name[ENUM_CAST(PositionModeConfig::Mode::LockIn)]) {
            daemonConfig.position_mode_config.mode = PositionModeConfig::Mode::LockIn;
        } else {
            daemonConfig.position_mode_config.mode = PositionModeConfig::Mode::Auto;
        }
        daemonConfig.position_mode_config.static_position.longitude = settings.lookup("geo_handling.static_coordinates.lon");
        daemonConfig.position_mode_config.static_position.latitude = settings.lookup("geo_handling.static_coordinates.lat");
        daemonConfig.position_mode_config.static_position.altitude = settings.lookup("geo_handling.static_coordinates.alt");
        daemonConfig.position_mode_config.static_position.hor_error = settings.lookup("geo_handling.static_coordinates.hor_error");
        daemonConfig.position_mode_config.static_position.vert_error = settings.lookup("geo_handling.static_coordinates.vert_error");
    } else {
        qFatal("error accessing settings. Aborting...");
        exit(EXIT_FAILURE);
    }

    daemonConfig.config_file_data.reset(&cfg);
    daemonConfig.settings_file_data.reset(&settings);

    if (verbose > 3) {
        qDebug() << "QT version is " << QString::number(QT_VERSION, 16);
    }

    Daemon daemon { daemonConfig };

    return a.exec();
}
