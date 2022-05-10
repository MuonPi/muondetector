#include "hardware/device_types.h"
#include "hardware/i2c/i2cutil.h"
#include "hardware/i2cdevices.h"
#include "utility/geohash.h"
#include "utility/gpio_mapping.h"
#include <QNetworkInterface>
#include <QThread>
#include <Qt>
#include <QtGlobal>
#include <QtNetwork>
#include <chrono>
#include <config.h>
#include <daemon.h>
#include <gpio_pin_definitions.h>
#include <iomanip>
#include <iostream>
#include <locale>
#include <logengine.h>
#include <muondetector_structs.h>
#include <set>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <tcpmessage_keys.h>
#include <time.h>
#include <ublox_messages.h>
#include <unistd.h>

#include <containers/gpio_config.h>
#include <containers/gpio_state.h>
#include <containers/bias_voltage.h>
#include <containers/dac_threshold.h>

#define DEGREE_CHARCODE 248

using namespace std;
using namespace MuonPi;

int64_t msecdiff(timespec& ts, timespec& st)
{
    int64_t diff;
    diff = (int64_t)ts.tv_sec - (int64_t)st.tv_sec;
    diff *= 1000;
    diff += ((int64_t)ts.tv_nsec - (int64_t)st.tv_nsec) / 1000000;
    return diff;
}

static QVector<uint16_t> allMsgCfgID({ UBX_MSG::TIM_TM2, UBX_MSG::TIM_TP,
    UBX_MSG::NAV_CLOCK, UBX_MSG::NAV_DGPS, UBX_MSG::NAV_AOPSTATUS, UBX_MSG::NAV_DOP,
    UBX_MSG::NAV_POSECEF, UBX_MSG::NAV_POSLLH, UBX_MSG::NAV_PVT, UBX_MSG::NAV_SBAS, UBX_MSG::NAV_SOL,
    UBX_MSG::NAV_STATUS, UBX_MSG::NAV_SVINFO, UBX_MSG::NAV_TIMEGPS, UBX_MSG::NAV_TIMEUTC, UBX_MSG::NAV_VELECEF,
    UBX_MSG::NAV_VELNED,
    UBX_MSG::MON_HW, UBX_MSG::MON_HW2, UBX_MSG::MON_IO, UBX_MSG::MON_MSGPP,
    UBX_MSG::MON_RXBUF, UBX_MSG::MON_RXR, UBX_MSG::MON_TXBUF });

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
        std::cout << "\nSIGTERM received" << std::endl;
    }

    // save ublox config in built-in flash to provide latest orbit info of sats for next start-up
    // and effectively reduce time-to-first-fix (TTFF)
    emit UBXSaveCfg();

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
        std::cout << "\nSIGHUP received" << std::endl;
    }

    // save ublox config in built-in flash to provide latest orbit info of sats for next start-up
    // and effectively reduce time-to-first-fix (TTFF)
    emit UBXSaveCfg();

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
        std::cout << "\nSIGINT received" << std::endl;
    }
    if (showin || showout) {
        qDebug() << allMsgCfgID.size();
        qDebug() << msgRateCfgs.size();
        for (QMap<uint16_t, int>::iterator it = msgRateCfgs.begin(); it != msgRateCfgs.end(); it++) {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
            qDebug().nospace() << "0x" << hex << (uint8_t)(it.key() >> 8) << " 0x" << hex << (uint8_t)(it.key() & 0xff) << " " << dec << it.value();
#else
            qDebug().nospace() << "0x" << Qt::hex << (uint8_t)(it.key() >> 8) << " 0x" << Qt::hex << (uint8_t)(it.key() & 0xff) << " " << Qt::dec << it.value();
#endif
        }
    }

    // save ublox config in built-in flash to provide latest orbit info of sats for next start-up
    // and effectively reduce time-to-first-fix (TTFF)
    emit UBXSaveCfg();

    emit aboutToQuit();
    exit(0);
    snInt->setEnabled(true);
}

Daemon::Daemon(configuration cfg, QObject* parent)
    : QTcpServer { parent }
    , config { cfg }
{
    // first, we must set the locale to be independent of the number format of the system's locale.
    // We rely on parsing floating point numbers with a decimal point (not a komma) which might fail if not setting the classic locale
    std::locale::global(std::locale::classic());

    qRegisterMetaType<TcpMessage>("TcpMessage");
    qRegisterMetaType<GeodeticPos>("GeodeticPos");
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

    // general
    verbose = config.verbose;
    if (verbose > 4) {
        qDebug() << "daemon running in thread " << this->thread()->objectName();
    }

    if (verbose > 3) {
        qDebug() << "QT version is " << QString::number(QT_VERSION, 16);
    }
    if (verbose > 3) {
        this->thread()->setObjectName("muondetector-daemon");
        qInfo() << this->thread()->objectName() << " thread id (pid): " << syscall(SYS_gettid);
    }

    // connect logParameter signal to log engine before anything else is done
    // since we want to log some initial one-time log parameters on start-up
    connect(this, &Daemon::logParameter, &logEngine, &LogEngine::onLogParameterReceived);

    // reset the I2C bus by issuing a general call reset
    I2cGeneralCall::resetDevices();

    // try to find out on which hardware version we are running
    // for this to work, we have to initialize and read the eeprom first
    // EEPROM 24AA02 type
    std::shared_ptr<EEPROM24AA02> eep24aa02_p;
    std::set<uint8_t> possible_addresses { 0x50 };
    auto found_dev_addresses = findI2cDeviceType<EEPROM24AA02>(possible_addresses);
    if (found_dev_addresses.size() > 0) {
        eep24aa02_p = std::make_shared<EEPROM24AA02>(found_dev_addresses.front());
    }
    if (eep24aa02_p && eep24aa02_p->identify()) {
        eep_p = std::static_pointer_cast<DeviceFunction<DeviceType::EEPROM>>(eep24aa02_p);
        std::cout << "eeprom " << eep24aa02_p->getName() << " identified at 0x" << std::hex << (int)eep24aa02_p->getAddress() << std::endl;
    } else {
        eep24aa02_p.reset();
        qCritical() << "eeprom device NOT found!";
    }

    calib = new ShowerDetectorCalib(eep24aa02_p);
    if (eep_p->probeDevicePresence()) {
        calib->readFromEeprom();
        if (verbose > 2) {
            // Caution, only for debugging. This code snippet writes a test sequence into the eeprom
            if (1 == 0) {
                uint8_t buf[256];
                for (int i = 0; i < 256; i++)
                    buf[i] = i;
                if (!eep24aa02_p->writeBytes(0, 256, buf))
                    std::cerr << "error: write to eeprom failed!" << std::endl;
                if (verbose > 2)
                    std::cout << "eep write took " << eep24aa02_p->getLastTimeInterval() << " ms" << std::endl;
                readEeprom();
            }
            if (1 == 1) {
                calib->printCalibList();
            }
        }
        uint64_t id = calib->getSerialID();
        QString hwIdStr = QString::number(id, 16);
        logParameter(LogParameter("uniqueId", hwIdStr, LogParameter::LOG_ONCE));
        hwIdStr = "0x" + hwIdStr;
        qInfo() << "EEP unique ID:" << hwIdStr;
    }
    CalibStruct verStruct = calib->getCalibItem("VERSION");
    unsigned int version = 0;
    ShowerDetectorCalib::getValueFromString(verStruct.value, version);
    if (version > 0) {
        MuonPi::Version::hardware.major = version;
        qInfo() << "Found HW version" << MuonPi::Version::hardware.major << "in eeprom";
    }

    // set up the pin definitions (hw version specific)
    GPIO_PINMAP = GPIO_PINMAP_VERSIONS[MuonPi::Version::hardware.major];

    if (verbose > 1) {
        // print out the current gpio pin mapping
        // (function, gpio-pin, direction)
        std::cout << "GPIO pin mapping:" << std::endl;

        for (auto signalIt = GPIO_PINMAP.begin(); signalIt != GPIO_PINMAP.end(); signalIt++) {
            const GPIO_SIGNAL signalId = signalIt->first;
            std::cout << GPIO_SIGNAL_MAP[signalId].name << " \t: " << std::dec << signalIt->second;
            switch (GPIO_SIGNAL_MAP[signalId].direction) {
            case DIR_IN:
                std::cout << " (in)";
                break;
            case DIR_OUT:
                std::cout << " (out)";
                break;
            case DIR_IO:
                std::cout << " (i/o)";
                break;
            default:
                std::cout << " (undef)";
            }
            std::cout << std::endl;
        }
    }

    mqttHandlerThread = new QThread();
    mqttHandlerThread->setObjectName("muondetector-daemon-mqtt");
    connect(this, &Daemon::aboutToQuit, mqttHandlerThread, &QThread::quit);
    connect(mqttHandlerThread, &QThread::finished, mqttHandlerThread, &QThread::deleteLater);

    mqttHandler = new MqttHandler(config.station_ID, verbose - 1);
    mqttHandler->moveToThread(mqttHandlerThread);
    connect(mqttHandler, &MqttHandler::connection_status, this, &Daemon::sendExtendedMqttStatus);
    //connect(mqttHandler, &MqttHandler::giving_up, this, &Daemon::handleSigTerm);
    //connect(mqttHandler, SIGNAL(giving_up()), this, SLOT(handleSigTerm()));
    connect(mqttHandlerThread, &QThread::finished, mqttHandler, &MqttHandler::deleteLater);
    connect(this, &Daemon::requestMqttConnectionStatus, mqttHandler, &MqttHandler::requestConnectionStatus);
    mqttHandlerThread->start();

    // create fileHandler
    fileHandlerThread = new QThread();
    fileHandlerThread->setObjectName("muondetector-daemon-filehandler");
    connect(this, &Daemon::aboutToQuit, fileHandlerThread, &QThread::quit);
    connect(fileHandlerThread, &QThread::finished, fileHandlerThread, &QThread::deleteLater);

    fileHandler = new FileHandler(config.username, config.password);
    fileHandler->moveToThread(fileHandlerThread);
    connect(this, &Daemon::aboutToQuit, fileHandler, &FileHandler::deleteLater);
    connect(fileHandlerThread, &QThread::started, fileHandler, &FileHandler::start);
    connect(fileHandlerThread, &QThread::finished, fileHandler, &FileHandler::deleteLater);
    connect(fileHandler, &FileHandler::mqttConnect, mqttHandler, &MqttHandler::start);
    fileHandlerThread->start();

    // connect log signals to and from log engine, file handler and mqtt handler
    connect(&logEngine, &LogEngine::sendLogString, mqttHandler,
        [this](const QString& content) {
            mqttHandler->publish(QString::fromStdString(Config::MQTT::log_topic), content);
        });
    connect(&logEngine, &LogEngine::sendLogString, fileHandler, &FileHandler::writeToLogFile);
    // connect to the regular log timer signal to log several non-regularly polled parameters
    connect(&logEngine, &LogEngine::logIntervalSignal, this, &Daemon::onLogParameterPolled);
    // connect the once-log flag reset slot of log engine with the logRotate signal of filehandler
    connect(fileHandler, &FileHandler::logRotateSignal, &logEngine, &LogEngine::onOnceLogTrigger);

    // instantiate, detect and initialize all other i2c devices

    // MIC184 or LM75 temp sensor
    possible_addresses = { 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f };
    found_dev_addresses = findI2cDeviceType<MIC184>(possible_addresses);
    if (found_dev_addresses.size() > 0) {
        temp_sensor_p = std::make_shared<MIC184>(found_dev_addresses.front());
    } else {
        // LM75A temp sensor
        found_dev_addresses = findI2cDeviceType<LM75>(possible_addresses);
        if (found_dev_addresses.size() > 0) {
            temp_sensor_p = std::make_shared<LM75>(found_dev_addresses.front());
        } else {
            qWarning() << "temp sensor NOT present!";
        }
    }

    if (temp_sensor_p && temp_sensor_p->probeDevicePresence()) {
        std::cout << "temp sensor " << temp_sensor_p->getName() << " identified at 0x" << std::hex << (int)dynamic_cast<i2cDevice*>(temp_sensor_p.get())->getAddress() << std::endl;
        if (verbose > 2) {
            std::cout << "function is " << temp_sensor_p->typeString() << std::endl;
            std::cout << "temperature is " << temp_sensor_p->getTemperature() << std::endl;
            std::cout << "readout took " << std::dec << dynamic_cast<i2cDevice*>(temp_sensor_p.get())->getLastTimeInterval() << "ms" << std::endl;
        }
        if (dynamic_cast<i2cDevice*>(temp_sensor_p.get())->getTitle() == "MIC184") {
            dynamic_cast<MIC184*>(temp_sensor_p.get())->setExternal(false);
        }
    }

    // detect and instantiate the I2C ADC ADS1015/1115
    std::shared_ptr<ADS1115> ads1115_p;
    possible_addresses = { 0x48, 0x49, 0x4a, 0x4b };
    found_dev_addresses = findI2cDeviceType<ADS1115>(possible_addresses);
    if (found_dev_addresses.size() > 0) {
        ads1115_p = std::make_shared<ADS1115>(found_dev_addresses.front());
        adc_p = std::static_pointer_cast<DeviceFunction<DeviceType::ADC>>(ads1115_p);
        std::cout << "ADS1115 identified at 0x" << std::hex << (int)ads1115_p->getAddress() << std::endl;
    } else {
        adcSamplingMode = ADC_SAMPLING_MODE::DISABLED;
        qWarning() << "ADS1115 device NOT present!";
    }

    if (ads1115_p && ads1115_p->probeDevicePresence()) {
        ads1115_p->setPga(ADS1115::PGA4V); // set full scale range to 4 Volts
        ads1115_p->setRate(ADS1115::SPS860); // set sampling rate to the maximum of 860 samples per second
        ads1115_p->setAGC(false); // turn AGC off for all channels
        if (!ads1115_p->setDataReadyPinMode()) {
            qWarning() << "error: failed setting data ready pin mode (setting thresh regs)";
        }

        // set up sampling timer used for continuous sampling mode
        samplingTimer.setInterval(Config::Hardware::trace_sampling_interval);
        samplingTimer.setSingleShot(false);
        samplingTimer.setTimerType(Qt::PreciseTimer);
        connect(&samplingTimer, &QTimer::timeout, this, &Daemon::sampleAdc0TraceEvent);

        // set up peak sampling mode
        setAdcSamplingMode(ADC_SAMPLING_MODE::PEAK);

        // set callback function for sample-ready events of the ADC
        adc_p->registerConversionReadyCallback([this](ADS1115::Sample sample) { this->onAdcSampleReady(sample); });

        if (verbose > 2) {
            bool ok = ads1115_p->setLowThreshold(0b0000000000000000);
            ok = ok && ads1115_p->setHighThreshold(-32768);
            if (ok)
                qDebug() << "successfully setting threshold registers";
            else
                qWarning() << "error: failed setting threshold registers";
            qDebug() << "single ended channels:";
            qDebug() << "ch0:" << ads1115_p->readADC(0) << "ch1:" << ads1115_p->readADC(1)
                     << "ch2: " << ads1115_p->readADC(2) << "ch3:" << ads1115_p->readADC(3);
            ads1115_p->setDiffMode(true);
            qDebug() << "diff channels:";
            qDebug() << "ch0-1:" << ads1115_p->readADC(0) << "ch0-3: " << ads1115_p->readADC(1)
                     << "ch1-3:" << ads1115_p->readADC(2) << "ch2-3: " << ads1115_p->readADC(3);
            ads1115_p->setDiffMode(false);
            qDebug() << "readout took" << ads1115_p->getLastTimeInterval() << "ms";
        }
    }

    // 4ch DAC MCP4728
    std::shared_ptr<MCP4728> mcp4728_p;
    possible_addresses = { 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67 };
    found_dev_addresses = findI2cDeviceType<MCP4728>(possible_addresses);
    if (found_dev_addresses.size() > 0) {
        mcp4728_p = std::make_shared<MCP4728>(found_dev_addresses.front());
        dac_p = std::static_pointer_cast<DeviceFunction<DeviceType::DAC>>(mcp4728_p);
        std::cout << "MCP4728 identified at 0x" << std::hex << (int)mcp4728_p->getAddress() << std::endl;
    } else {
        qFatal("MCP4728 device NOT present!");
        // this error is critical, since the whole concept of counting muons is based on
        // the function of the threshold discriminator
        // we should quit here returning an error code (?)
    }

    if (mcp4728_p && mcp4728_p->probeDevicePresence()) {
        //mcp4728_p->setVRef( MCP4728::VREF_VDD );
        if (verbose > 2) {
            qInfo() << "MCP4728 device is present.";
            qDebug() << "DAC registers / output voltages:";
            for (int i = 0; i < 4; i++) {
                MCP4728::DacChannel dacChannel;
                MCP4728::DacChannel eepromChannel;
                eepromChannel.eeprom = true;
                mcp4728_p->readChannel(i, dacChannel);
                mcp4728_p->readChannel(i, eepromChannel);
                qDebug() << " ch" << i << ": " << dacChannel.value << "=" << MCP4728::code2voltage(dacChannel) << "V"
                         << "  (stored:" << eepromChannel.value << "=" << MCP4728::code2voltage(eepromChannel) << "V)";
            }
            qDebug() << "readout took" << mcp4728_p->getLastTimeInterval() << "ms";
        }
    }

    for (int channel = 0; channel < 2; channel++) {
        if (config.thresholdVoltage[Config::Hardware::DAC::Channel::threshold[channel]] < 0.
            && mcp4728_p->probeDevicePresence()) {
            MCP4728::DacChannel dacChannel;
            MCP4728::DacChannel eepromChannel;
            eepromChannel.eeprom = true;
            mcp4728_p->readChannel(Config::Hardware::DAC::Channel::threshold[channel], dacChannel);
            mcp4728_p->readChannel(Config::Hardware::DAC::Channel::threshold[channel], eepromChannel);
            config.thresholdVoltage[Config::Hardware::DAC::Channel::threshold[channel]] = MCP4728::code2voltage(dacChannel);
        }
    }

    if (mcp4728_p->probeDevicePresence()) {
        MCP4728::DacChannel eeprom_value;
        eeprom_value.eeprom = true;
        mcp4728_p->readChannel(Config::Hardware::DAC::Channel::bias, eeprom_value);
        if (eeprom_value.value == 0) {
            setBiasVoltage(Config::Hardware::DAC::Voltage::bias);
            setDacThresh(Config::Hardware::DAC::Channel::threshold[0], Config::Hardware::DAC::Voltage::threshold[0]);
            setDacThresh(Config::Hardware::DAC::Channel::threshold[1], Config::Hardware::DAC::Voltage::threshold[1]);
        }
    }

    if (config.biasVoltage < 0. && mcp4728_p->probeDevicePresence()) {
        MCP4728::DacChannel dacChannel;
        MCP4728::DacChannel eepromChannel;
        eepromChannel.eeprom = true;
        mcp4728_p->readChannel(Config::Hardware::DAC::Channel::bias, dacChannel);
        mcp4728_p->readChannel(Config::Hardware::DAC::Channel::bias, eepromChannel);
        config.biasVoltage = MCP4728::code2voltage(dacChannel);
    }

    // PCA9536 4 bit I/O I2C device used for selecting the UBX timing input
    std::shared_ptr<PCA9536> pca9536_p;
    possible_addresses = { 0x41 };
    found_dev_addresses = findI2cDeviceType<PCA9536>(possible_addresses);
    if (found_dev_addresses.size() > 0) {
        pca9536_p = std::make_shared<PCA9536>(found_dev_addresses.front());
        io_extender_p = std::static_pointer_cast<DeviceFunction<DeviceType::IO_EXTENDER>>(pca9536_p);
        std::cout << "PCA9536 identified at 0x" << std::hex << (int)pca9536_p->getAddress() << std::endl;
    } else {
        qWarning() << "PCA9536 device NOT found!";
    }

    if (io_extender_p && io_extender_p->probeDevicePresence()) {
        pca9536_p->identify();
        if (verbose > 2) {
            qInfo() << "PCA9536 device is present.";
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
            qDebug().nospace() << " inputs: 0x" << hex << (int)io_extender_p->getInputState();
            qDebug() << "readout took" << dec << pca9536_p->getLastTimeInterval() << "ms";
#else
            qDebug().nospace() << " inputs: 0x" << Qt::hex << (int)io_extender_p->getInputState();
            qDebug() << "readout took " << Qt::dec << pca9536_p->getLastTimeInterval() << "ms";
#endif
        }
        io_extender_p->setOutputPorts(0b00000111);
        setPcaChannel(config.pcaPortMask);
    }

    if (mcp4728_p->probeDevicePresence()) {
        if (config.thresholdVoltage[0] > 0.)
            dac_p->setVoltage(Config::Hardware::DAC::Channel::threshold[0], config.thresholdVoltage[0]);
        if (config.thresholdVoltage[1] > 0.)
            dac_p->setVoltage(Config::Hardware::DAC::Channel::threshold[1], config.thresholdVoltage[1]);
        if (config.biasVoltage > 0)
            dac_p->setVoltage(Config::Hardware::DAC::Channel::bias, config.biasVoltage);
    }

    // removed initialization of ublox i2c interface since it doesn't work properly on RPi
    // the Ublox i2c relies on clock stretching, which RPi is not supporting
    // the ublox's i2c address is still occupied but locked, i.e. access is prohibited
    ublox_i2c_p = std::make_shared<UbloxI2c>(0x42);
    ublox_i2c_p->lock();
    if ((verbose > 2) && ublox_i2c_p->devicePresent()) {
        qInfo() << "ublox I2C device interface is present.";
        uint16_t bufcnt = 0;
        bool ok = ublox_i2c_p->getTxBufCount(bufcnt);
        if (ok)
            qDebug() << "bytes in TX buf:" << bufcnt;
    }

    // check if also an Adafruit-SSD1306 compatible i2c OLED display is present
    // initialize and start loop for display of several state variables
    oled_p = std::make_shared<Adafruit_SSD1306>(0x3c);
    if (!oled_p->devicePresent()) {
        oled_p = std::make_shared<Adafruit_SSD1306>(0x3d);
    }
    if (oled_p->devicePresent()) {
        if (verbose > -1) {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
            qInfo() << "I2C SSD1306-type OLED display found at address 0x" << hex << oled_p->getAddress();
#else
            qInfo() << "I2C SSD1306-type OLED display found at address 0x" << Qt::hex << oled_p->getAddress();
#endif
        }
        oled_p->begin();
        oled_p->clearDisplay();

        // text display tests
        oled_p->setTextSize(1);
        oled_p->setTextColor(Adafruit_SSD1306::WHITE);
        oled_p->setCursor(0, 2);
        oled_p->print("*Cosmic Shower Det.*\n");
        oled_p->print("V");
        oled_p->print(Version::software.string().c_str());
        oled_p->print("\n");
        oled_p->display();
        usleep(50000L);
        connect(&oledUpdateTimer, SIGNAL(timeout()), this, SLOT(updateOledDisplay()));

        oledUpdateTimer.start(Config::Hardware::OLED::update_interval);
    } else {
        oled_p.reset();
    }

    // for diagnostics:
    // print out some i2c device statistics
    if (verbose > 2) {
        std::cout << "Nr. of invoked I2C devices (plain count): " << std::dec << i2cDevice::getNrDevices() << std::endl;
        std::cout << "Nr. of invoked I2C devices (gl. device list's size): " << i2cDevice::getGlobalDeviceList().size() << std::endl;
        std::cout << "Nr. of bytes read on I2C bus: " << i2cDevice::getGlobalNrBytesRead() << std::endl;
        std::cout << "Nr. of bytes written on I2C bus: " << i2cDevice::getGlobalNrBytesWritten() << std::endl;
        std::cout << "list of device addresses: " << std::endl;
        for (uint8_t i = 0; i < i2cDevice::getGlobalDeviceList().size(); i++) {
            std::cout << (int)i + 1 << " 0x" << std::hex << (int)i2cDevice::getGlobalDeviceList()[i]->getAddress() << " " << i2cDevice::getGlobalDeviceList()[i]->getTitle();
            if (i2cDevice::getGlobalDeviceList()[i]->devicePresent())
                std::cout << " present" << endl;
            else
                std::cout << " missing" << endl;
        }
        if (temp_sensor_p)
            dynamic_cast<i2cDevice*>(temp_sensor_p.get())->getCapabilities();
    }

    // for ublox gnss module
    gpsdevname = config.gpsdevname;
    showout = config.showout;
    showin = config.showin;

    // for tcp connection with fileServer
    peerPort = config.peerPort;
    if (peerPort == 0) {
        peerPort = 51508;
    }
    peerAddress = config.peerAddress;
    if (peerAddress.isEmpty() || peerAddress == "local" || peerAddress == "localhost") {
        peerAddress = QHostAddress(QHostAddress::LocalHost).toString();
    }

    if (config.serverAddress.isEmpty()) {
        // if not otherwise specified: listen on all available addresses
        daemonAddress = QHostAddress(QHostAddress::Any);
        if (verbose > 3) {
            qDebug() << "daemon address: " << daemonAddress.toString();
        }
    }
    daemonPort = config.serverPort;
    if (daemonPort == 0) {
        // maybe think about other fall back solution
        daemonPort = Settings::gui.port;
    }
    if (!this->listen(daemonAddress, daemonPort)) {
        qCritical() << tr("Unable to start the server: %1.\n").arg(this->errorString());
    } else {
        if (verbose > 3) {
            qInfo() << tr("\nThe server is running on\n\nIP: %1\nport: %2\n")
                           .arg(daemonAddress.toString())
                           .arg(serverPort());
        }
    }
    std::flush(std::cout);

    // connect to the pigpio daemon interface for gpio control
    connectToPigpiod();

    // set up histograms
    setupHistos();

    // establish ublox gnss module connection
    connectToGps();

    // configure the ublox module with preset ubx messages, if required
    if (config.gnss_config) {
        configGps();
    }
    pollAllUbxMsgRate();

    // set up cyclic timer monitoring following operational parameters:
    // temp, vadc, vbias, ibias
    parameterMonitorTimer.setInterval(Config::Hardware::monitor_interval);
    parameterMonitorTimer.setSingleShot(false);
    connect(&parameterMonitorTimer, &QTimer::timeout, this, &Daemon::aquireMonitoringParameters);
    parameterMonitorTimer.start();

    emit logParameter(LogParameter("maxGeohashLength", QString::number(config.maxGeohashLength), LogParameter::LOG_ONCE));
    emit logParameter(LogParameter("softwareVersionString", QString::fromStdString(MuonPi::Version::software.string()), LogParameter::LOG_ONCE));
    emit logParameter(LogParameter("hardwareVersionString", QString::fromStdString(MuonPi::Version::hardware.string()), LogParameter::LOG_ONCE));
}

Daemon::~Daemon()
{
    snHup.clear();
    snTerm.clear();
    snInt.clear();
    if (calib != nullptr) {
        delete calib;
        calib = nullptr;
    }
    pigHandler.clear();
    constexpr unsigned long timeout { 2000 };
    if (!mqttHandlerThread->wait(timeout)) {
        qWarning() << "Timeout waiting for thread" + mqttHandlerThread->objectName();
    }
    if (!fileHandlerThread->wait(timeout)) {
        qWarning() << "Timeout waiting for thread" + fileHandlerThread->objectName();
    }
    if (!pigThread->wait(timeout)) {
        qWarning() << "Timeout waiting for thread" + pigThread->objectName();
    }
    if (!gpsThread->wait(timeout)) {
        qWarning() << "Timeout waiting for thread" + gpsThread->objectName();
    }
    if (!tcpThread->wait(timeout)) {
        qWarning() << "Timeout waiting for thread" + tcpThread->objectName();
    }
    while (!i2cDevice::getGlobalDeviceList().empty()) {
        if (i2cDevice::getGlobalDeviceList().front() != nullptr)
            delete i2cDevice::getGlobalDeviceList().front();
    }
}

void Daemon::onMessageReceived(std::shared_ptr<message_container>) {

}

void Daemon::connectToPigpiod()
{
    const QVector<unsigned int> gpio_pins({ GPIO_PINMAP[EVT_AND], GPIO_PINMAP[EVT_XOR],
        GPIO_PINMAP[TIMEPULSE], GPIO_PINMAP[EXT_TRIGGER] });
    pigHandler = new PigpiodHandler(gpio_pins);
    tdc7200 = new TDC7200(GPIO_PINMAP[TDC_INTB]);
    pigThread = new QThread();
    pigThread->setObjectName("muondetector-daemon-pigpio");
    pigHandler->moveToThread(pigThread);
    tdc7200->moveToThread(pigThread);
    connect(this, &Daemon::aboutToQuit, pigThread, &QThread::quit);
    connect(pigThread, &QThread::finished, pigThread, &QThread::deleteLater);

    //pighandler <-> tdc
    connect(pigHandler, &PigpiodHandler::spiData, tdc7200, &TDC7200::onDataReceived);
    connect(pigHandler, &PigpiodHandler::signal, tdc7200, &TDC7200::onDataAvailable);
    connect(tdc7200, &TDC7200::readData, pigHandler, &PigpiodHandler::readSpi);
    connect(tdc7200, &TDC7200::writeData, pigHandler, &PigpiodHandler::writeSpi);

    //tdc <-> thread & daemon
    connect(tdc7200, &TDC7200::tdcEvent, this, [this](double usecs) {
        if (histoMap.find("Time-to-Digital Time Diff") != histoMap.end()) {
            histoMap["Time-to-Digital Time Diff"].fill(usecs);
        }
    });
    connect(tdc7200, &TDC7200::statusUpdated, this, [this](bool isPresent) {
        spiDevicePresent = isPresent;
        sendSpiStats();
    });
    connect(pigThread, &QThread::started, tdc7200, &TDC7200::initialise);
    connect(pigThread, &QThread::finished, tdc7200, &TDC7200::deleteLater);

    //pigHandler <-> thread & daemon
    connect(this, &Daemon::aboutToQuit, pigHandler, &PigpiodHandler::stop);
    connect(pigThread, &QThread::finished, pigHandler, &PigpiodHandler::deleteLater);
    connect(this, &Daemon::message, pigHandler, &PigpiodHandler::onMessageReceived);
    connect(this, &Daemon::GpioRegisterForCallback, pigHandler, &PigpiodHandler::registerForCallback);
    connect(pigHandler, &PigpiodHandler::signal, this, &Daemon::sendGpioPinEvent);

    connect(pigHandler, &PigpiodHandler::signal, this, [this](uint8_t gpio_pin) {
        if (gpio_pin == GPIO_PINMAP[TIME_MEAS_OUT]) {
            onStatusLed2Event(50);
        }
    });

    connect(pigHandler, &PigpiodHandler::samplingTrigger, this, &Daemon::sampleAdc0Event);
    connect(pigHandler, &PigpiodHandler::eventInterval, this, [this](quint64 nsecs) {
        if (histoMap.find("gpioEventInterval") != histoMap.end()) {
            histoMap["gpioEventInterval"].fill(1e-6 * nsecs);
        }
    });
    connect(pigHandler, &PigpiodHandler::eventInterval, this, [this](quint64 nsecs) {
        if (histoMap.find("gpioEventIntervalShort") != histoMap.end()) {
            if (nsecs / 1000 <= histoMap["gpioEventIntervalShort"].getMax())
                histoMap["gpioEventIntervalShort"].fill((double)nsecs / 1000.);
        }
    });
    connect(pigHandler, &PigpiodHandler::timePulseDiff, this, [this](qint32 usecs) {
        if (histoMap.find("TPTimeDiff") != histoMap.end()) {
            histoMap["TPTimeDiff"].fill((double)usecs);
        }
    });
    pigHandler->setSamplingTriggerSignal(config.eventTrigger);
    connect(this, &Daemon::setSamplingTriggerSignal, pigHandler, &PigpiodHandler::setSamplingTriggerSignal);

    struct timespec ts_res;
    clock_getres(CLOCK_REALTIME, &ts_res);
    if (verbose > 3) {
        qInfo() << "the timing resolution of the system clock is" << ts_res.tv_nsec << "ns";
    }

    timespec_get(&lastRateInterval, TIME_UTC);
    startOfProgram = lastRateInterval;
    connect(pigHandler, &PigpiodHandler::signal, this, [this](uint8_t gpio_pin) {
        rateCounterIntervalActualisation();
        if (gpio_pin == GPIO_PINMAP[EVT_XOR]) {
            xorCounts.back()++;
        }
        if (gpio_pin == GPIO_PINMAP[EVT_AND]) {
            andCounts.back()++;
        }
    });
    pigThread->start();
    rateBufferReminder.setInterval(rateBufferInterval);
    rateBufferReminder.setSingleShot(false);
    connect(&rateBufferReminder, &QTimer::timeout, this, &Daemon::onRateBufferReminder);
    rateBufferReminder.start();

    gpio_config f_gpio_config{{
        {GPIO_PINMAP[UBIAS_EN], gpio_config::output},
        {GPIO_PINMAP[PREAMP_1], gpio_config::output},
        {GPIO_PINMAP[PREAMP_2], gpio_config::output},
        {GPIO_PINMAP[GAIN_HL], gpio_config::output},
        {GPIO_PINMAP[STATUS1], gpio_config::output},
        {GPIO_PINMAP[STATUS2], gpio_config::output},
        {GPIO_PINMAP[ADC_READY], gpio_config::pullup}
    }};
    std::vector<std::shared_ptr<gpio_state>> gpio_states{};
    gpio_states.push_back(std::make_shared<gpio_state>(GPIO_PINMAP[UBIAS_EN], (MuonPi::Version::hardware.major == 1) ? config.bias_ON : !config.bias_ON));

    gpio_states.push_back(std::make_shared<gpio_state>(GPIO_PINMAP[PREAMP_1], config.preamp_enable[0]));
    gpio_states.push_back(std::make_shared<gpio_state>(GPIO_PINMAP[PREAMP_2], config.preamp_enable[1]));
    gpio_states.push_back(std::make_shared<gpio_state>(GPIO_PINMAP[GAIN_HL], config.hi_gain));
    gpio_states.push_back(std::make_shared<gpio_state>(GPIO_PINMAP[STATUS1], 0));
    gpio_states.push_back(std::make_shared<gpio_state>(GPIO_PINMAP[STATUS2], 0));
    emit GpioRegisterForCallback(GPIO_PINMAP[ADC_READY], 1);

    if (MuonPi::Version::hardware.major > 1) {
        f_gpio_config.emplace(GPIO_PINMAP[PREAMP_FAULT], gpio_config::pullup);
        f_gpio_config.emplace(GPIO_PINMAP[TDC_INTB], gpio_config::input);
        f_gpio_config.emplace(GPIO_PINMAP[TIME_MEAS_OUT], gpio_config::input);
        emit GpioRegisterForCallback(GPIO_PINMAP[PREAMP_FAULT], 0);
        emit GpioRegisterForCallback(GPIO_PINMAP[TDC_INTB], 0);
        emit GpioRegisterForCallback(GPIO_PINMAP[TIME_MEAS_OUT], 0);
    }
    if (MuonPi::Version::hardware.major >= 3) {
        f_gpio_config.emplace(GPIO_PINMAP[IN_POL1], gpio_config::output);
        f_gpio_config.emplace(GPIO_PINMAP[IN_POL2], gpio_config::output);
        gpio_states.push_back(std::make_shared<gpio_state>(GPIO_PINMAP[IN_POL1], config.polarity[0]));
        gpio_states.push_back(std::make_shared<gpio_state>(GPIO_PINMAP[IN_POL2], config.polarity[1]));
    }
    emit message(std::make_shared<gpio_config>(f_gpio_config));
    for (auto state : gpio_states){
        emit message(state);
    }
}

void Daemon::connectToGps()
{
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
    qtGps = new QtSerialUblox(gpsdevname, gpsTimeout, config.gnss_baudrate, config.gnss_dump_raw, verbose - 1, showout, showin);
    gpsThread = new QThread();
    gpsThread->setObjectName("muondetector-daemon-gnss");
    qtGps->moveToThread(gpsThread);
    // connect all signals about quitting
    connect(this, &Daemon::aboutToQuit, gpsThread, &QThread::quit);
    connect(this, &Daemon::aboutToQuit, qtGps, &QtSerialUblox::closeAll);
    connect(gpsThread, &QThread::finished, gpsThread, &QThread::deleteLater);
    connect(gpsThread, &QThread::finished, qtGps, &QtSerialUblox::deleteLater);
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
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedGeodeticPos, this, &Daemon::onGpsPropertyUpdatedGeodeticPos);
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedGnss, this, &Daemon::onGpsPropertyUpdatedGnss);
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedUint32, this, &Daemon::gpsPropertyUpdatedUint32);
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedInt32, this, &Daemon::gpsPropertyUpdatedInt32);
    connect(qtGps, &QtSerialUblox::gpsPropertyUpdatedUint8, this, &Daemon::gpsPropertyUpdatedUint8);
    connect(qtGps, &QtSerialUblox::gpsMonHW, this, &Daemon::onGpsMonHWUpdated);
    connect(qtGps, &QtSerialUblox::gpsMonHW2, this, &Daemon::onGpsMonHW2Updated);
    connect(qtGps, &QtSerialUblox::gpsVersion, this, &Daemon::UBXReceivedVersion);
    connect(qtGps, &QtSerialUblox::UBXCfgError, this, &Daemon::toConsole);
    connect(qtGps, &QtSerialUblox::UBXReceivedGnssConfig, this, &Daemon::onUBXReceivedGnssConfig);
    connect(qtGps, &QtSerialUblox::UBXReceivedTP5, this, &Daemon::onUBXReceivedTP5);
    connect(qtGps, &QtSerialUblox::UBXReceivedTxBuf, this, &Daemon::onUBXReceivedTxBuf);
    connect(qtGps, &QtSerialUblox::UBXReceivedRxBuf, this, &Daemon::onUBXReceivedRxBuf);
    connect(this, &Daemon::UBXSetDynModel, qtGps, &QtSerialUblox::setDynamicModel);
    connect(this, &Daemon::resetUbxDevice, qtGps, &QtSerialUblox::UBXReset);
    connect(this, &Daemon::setGnssConfig, qtGps, &QtSerialUblox::onSetGnssConfig);
    connect(this, &Daemon::UBXSetCfgTP5, qtGps, &QtSerialUblox::UBXSetCfgTP5);
    connect(this, &Daemon::UBXSetMinMaxSVs, qtGps, &QtSerialUblox::UBXSetMinMaxSVs);
    connect(this, &Daemon::UBXSetMinCNO, qtGps, &QtSerialUblox::UBXSetMinCNO);
    connect(this, &Daemon::UBXSetAopCfg, qtGps, &QtSerialUblox::UBXSetAopCfg);
    connect(this, &Daemon::UBXSaveCfg, qtGps, &QtSerialUblox::UBXSaveCfg);
    connect(qtGps, &QtSerialUblox::UBXReceivedTimeTM2, this, &Daemon::onUBXReceivedTimeTM2);

    connect(qtGps, &QtSerialUblox::UBXReceivedDops, this, [this](const UbxDopStruct& dops) {
        currentDOP = dops;
        emit logParameter(LogParameter("positionDOP", QString::number(dops.pDOP / 100.), LogParameter::LOG_AVERAGE));
        emit logParameter(LogParameter("timeDOP", QString::number(dops.tDOP / 100.), LogParameter::LOG_AVERAGE));
    });

    // connect fileHandler related stuff
    if (fileHandler != nullptr) {
        qDebug() << "store_local flag =" << config.storeLocal;

        if (config.storeLocal) {
            connect(this, &Daemon::eventMessage, fileHandler, &FileHandler::writeToDataFile);
        }
        connect(this, &Daemon::eventMessage, mqttHandler,
            [this](const QString& content) {
                mqttHandler->publish(QString::fromStdString(Config::MQTT::data_topic), content);
            });
    }
    // after thread start there will be a signal emitted which starts the qtGps makeConnection function
    gpsThread->start();
}

void Daemon::incomingConnection(qintptr socketDescriptor)
{
    if (verbose > 4) {
        qDebug() << "incoming connection";
    }
    tcpThread = new QThread();
    tcpThread->setObjectName("muondetector-daemon-tcp");
    tcpConnection = new TcpConnection(socketDescriptor, verbose);
    tcpConnection->moveToThread(tcpThread);
    // connect all signals about quitting
    connect(this, &Daemon::aboutToQuit, tcpConnection, &TcpConnection::closeThisConnection);
    connect(this, &Daemon::closeConnection, tcpConnection, &TcpConnection::closeConnection);
    connect(tcpConnection, &TcpConnection::finished, tcpThread, &QThread::quit);
    connect(tcpThread, &QThread::finished, tcpThread, &QThread::deleteLater);
    connect(tcpThread, &QThread::finished, tcpConnection, &TcpConnection::deleteLater);
    // connect all other signals
    connect(tcpThread, &QThread::started, tcpConnection, &TcpConnection::receiveConnection);
    connect(this, &Daemon::message, tcpConnection, &TcpConnection::onMessageSignal);
    connect(this, &Daemon::sendTcpMessage, tcpConnection, &TcpConnection::sendTcpMessage);
    connect(tcpConnection, &TcpConnection::receivedTcpMessage, this, &Daemon::receivedTcpMessage);
    connect(tcpConnection, &TcpConnection::toConsole, this, &Daemon::toConsole);
    connect(tcpConnection, &TcpConnection::madeConnection, this, &Daemon::onMadeConnection);
    connect(tcpConnection, &TcpConnection::connectionTimeout, this, &Daemon::onStoppedConnection);
    tcpThread->start();

    pollAllUbxMsgRate();
    emit requestMqttConnectionStatus();
}

// Histogram functions
void Daemon::setupHistos()
{
    Histogram hist = Histogram("geoHeight", 200, 0., 199.);
    hist.setUnit("m");
    hist.setAutoscale();
    histoMap["geoHeight"] = hist;
    hist = Histogram("geoLongitude", 200, 0., 0.003);
    hist.setUnit("deg");
    hist.setAutoscale();
    histoMap["geoLongitude"] = hist;
    hist = Histogram("geoLatitude", 200, 0., 0.003);
    hist.setUnit("deg");
    hist.setAutoscale();
    histoMap["geoLatitude"] = hist;
    hist = Histogram("weightedGeoHeight", 200, 0., 199.);
    hist.setUnit("m");
    hist.setAutoscale();
    histoMap["weightedGeoHeight"] = hist;
    hist = Histogram("pulseHeight", 500, 0., 3.8);
    hist.setUnit("V");
    hist.setAutoscale(false);
    histoMap["pulseHeight"] = hist;
    hist = Histogram("adcSampleTime", 500, 0., 10.0);
    hist.setUnit("ms");
    hist.setAutoscale();
    histoMap["adcSampleTime"] = hist;
    hist = Histogram("UbxEventLength", 100, 50., 149.);
    hist.setUnit("ns");
    hist.setAutoscale();
    histoMap["UbxEventLength"] = hist;
    hist = Histogram("gpioEventInterval", 400, 0., 2000.);
    hist.setUnit("ms");
    hist.setAutoscale();
    histoMap["gpioEventInterval"] = hist;
    hist = Histogram("gpioEventIntervalShort", 50, 0., 49.);
    hist.setUnit("us");
    hist.setAutoscale(false);
    histoMap["gpioEventIntervalShort"] = hist;
    hist = Histogram("UbxEventInterval", 200, 0., 2000.);
    hist.setUnit("ms");
    hist.setAutoscale();
    histoMap["UbxEventInterval"] = hist;
    hist = Histogram("TPTimeDiff", 200, -999., 1000.);
    hist.setUnit("us");
    hist.setAutoscale();
    histoMap["TPTimeDiff"] = hist;
    hist = Histogram("Time-to-Digital Time Diff", 400, 0., 1e6);
    hist.setUnit("ns");
    hist.setAutoscale();
    histoMap["Time-to-Digital Time Diff"] = hist;
    hist = Histogram("Bias Voltage", 200, 0., 1.);
    hist.setUnit("V");
    hist.setAutoscale();
    histoMap["Bias Voltage"] = hist;
    hist = Histogram("Bias Current", 200, 0., 50.);
    hist.setUnit("uA");
    hist.setAutoscale();
    histoMap["Bias Current"] = hist;
}

void Daemon::clearHisto(const QString& histoName)
{
    if (histoMap.find(histoName) != histoMap.end()) {
        histoMap[histoName].clear();
        emit sendHistogram(histoMap[histoName]);
    }
    return;
}

// ALL FUNCTIONS ABOUT TCPMESSAGE SENDING AND RECEIVING
void Daemon::receivedTcpMessage(TcpMessage tcpMessage)
{
    TCP_MSG_KEY msgID = static_cast<TCP_MSG_KEY>(tcpMessage.getMsgID());
    if (msgID == TCP_MSG_KEY::MSG_THRESHOLD) {
        uint8_t channel;
        float threshold;
        *(tcpMessage.dStream) >> channel >> threshold;
        if (channel > 1)
            return;
        if (threshold < 0.001) {
            if (verbose > 2)
                qWarning() << "setting DAC" << channel << "to 0!";
        } else
            setDacThresh(channel, threshold);
        emit message(std::make_shared<dac_threshold>(channel, Config::Hardware::DAC::Channel::threshold[channel]));
    } else if (msgID == TCP_MSG_KEY::MSG_THRESHOLD_REQUEST) {
        emit message(std::make_shared<dac_threshold>(0, Config::Hardware::DAC::Channel::threshold[0]));
        emit message(std::make_shared<dac_threshold>(1, Config::Hardware::DAC::Channel::threshold[1]));
    } else if (msgID == TCP_MSG_KEY::MSG_BIAS_VOLTAGE) {
        float voltage;
        *(tcpMessage.dStream) >> voltage;
        setBiasVoltage(voltage);
        if (histoMap.find("pulseHeight") != histoMap.end())
            histoMap["pulseHeight"].clear();
        emit message(std::make_shared<bias_voltage>(config.biasVoltage));
    } else if (msgID == TCP_MSG_KEY::MSG_BIAS_VOLTAGE_REQUEST) {
        emit message(std::make_shared<bias_voltage>(config.biasVoltage));
    } else if (msgID == TCP_MSG_KEY::MSG_BIAS_SWITCH) {
        bool status;
        *(tcpMessage.dStream) >> status;
        setBiasStatus(status);
        emit message(std::make_shared<bias_voltage>(config.biasVoltage));
    } else if (msgID == TCP_MSG_KEY::MSG_BIAS_SWITCH_REQUEST) {
        emit message(std::make_shared<bias_voltage>(config.biasVoltage));
    } else if (msgID == TCP_MSG_KEY::MSG_BIAS_VOLTAGE_REQUEST) {
        emit message(std::make_shared<bias_voltage>(config.biasVoltage));
    } else if (msgID == TCP_MSG_KEY::MSG_PREAMP_SWITCH) {
        quint8 channel;
        bool status;
        *(tcpMessage.dStream) >> channel >> status;
        if (channel == 0) {
            config.preamp_enable[0] = status;
            emit message(std::make_shared<gpio_state>(GPIO_PINMAP[PREAMP_1], status));
            emit logParameter(LogParameter("preampSwitch1", QString::number((int)config.preamp_enable[0]), LogParameter::LOG_EVERY));
        } else if (channel == 1) {
            config.preamp_enable[1] = status;
            emit message(std::make_shared<gpio_state>(GPIO_PINMAP[PREAMP_2], status));
            emit logParameter(LogParameter("preampSwitch2", QString::number((int)config.preamp_enable[1]), LogParameter::LOG_EVERY));
        }
        sendPreampStatus(0);
        sendPreampStatus(1);
    } else if (msgID == TCP_MSG_KEY::MSG_PREAMP_SWITCH_REQUEST) {
        sendPreampStatus(0);
        sendPreampStatus(1);
    } else if (msgID == TCP_MSG_KEY::MSG_POLARITY_SWITCH) {
        bool pol1, pol2;
        *(tcpMessage.dStream) >> pol1 >> pol2;
        if (MuonPi::Version::hardware.major >= 3 && pol1 != config.polarity[0]) {
            config.polarity[0] = pol1;
            emit message(std::make_shared<gpio_state>(GPIO_PINMAP[IN_POL1], config.polarity[0]));
            emit logParameter(LogParameter("polaritySwitch1", QString::number((int)config.polarity[0]), LogParameter::LOG_EVERY));
        }
        if (MuonPi::Version::hardware.major >= 3 && pol2 != config.polarity[1]) {
            config.polarity[1] = pol2;
            emit message(std::make_shared<gpio_state>(GPIO_PINMAP[IN_POL2], config.polarity[1]));
            emit logParameter(LogParameter("polaritySwitch2", QString::number((int)config.polarity[1]), LogParameter::LOG_EVERY));
        }
        sendPolarityStatus();
    } else if (msgID == TCP_MSG_KEY::MSG_POLARITY_SWITCH_REQUEST) {
        sendPolarityStatus();
    } else if (msgID == TCP_MSG_KEY::MSG_GAIN_SWITCH) {
        bool status;
        *(tcpMessage.dStream) >> status;
        config.hi_gain = status;
        emit message(std::make_shared<gpio_state>(GPIO_PINMAP[GAIN_HL], status));
        if (histoMap.find("pulseHeight") != histoMap.end())
            histoMap["pulseHeight"].clear();
        emit logParameter(LogParameter("gainSwitch", QString::number((int)config.hi_gain), LogParameter::LOG_EVERY));
        sendGainSwitchStatus();
        return;
    } else if (msgID == TCP_MSG_KEY::MSG_GAIN_SWITCH_REQUEST) {
        sendGainSwitchStatus();
    } else if (msgID == TCP_MSG_KEY::MSG_UBX_MSG_RATE_REQUEST) {
        sendUbxMsgRates();
        return;
    } else if (msgID == TCP_MSG_KEY::MSG_UBX_RESET) {
        uint32_t resetFlags = QtSerialUblox::RESET_WARM | QtSerialUblox::RESET_SW;
        emit resetUbxDevice(resetFlags);
        pollAllUbxMsgRate();
    } else if (msgID == TCP_MSG_KEY::MSG_UBX_CONFIG_DEFAULT) {
        configGps();
        pollAllUbxMsgRate();
    } else if (msgID == TCP_MSG_KEY::MSG_UBX_MSG_RATE) {
        QMap<uint16_t, int> ubxMsgRates;
        *(tcpMessage.dStream) >> ubxMsgRates;
        setUbxMsgRates(ubxMsgRates);
    } else if (msgID == TCP_MSG_KEY::MSG_PCA_SWITCH) {
        quint8 portMask;
        *(tcpMessage.dStream) >> portMask;
        setPcaChannel((uint8_t)portMask);
        sendPcaChannel();
        if (histoMap.find("UbxEventLength") != histoMap.end())
            histoMap["UbxEventLength"].clear();
    } else if (msgID == TCP_MSG_KEY::MSG_PCA_SWITCH_REQUEST) {
        sendPcaChannel();
    } else if (msgID == TCP_MSG_KEY::MSG_EVENTTRIGGER) {
        unsigned int signal;
        *(tcpMessage.dStream) >> signal;
        setEventTriggerSelection((GPIO_SIGNAL)signal);
        usleep(1000);
        sendEventTriggerSelection();
        return;
    } else if (msgID == TCP_MSG_KEY::MSG_EVENTTRIGGER_REQUEST) {
        sendEventTriggerSelection();
    } else if (msgID == TCP_MSG_KEY::MSG_GPIO_RATE_REQUEST) {
        quint8 whichRate;
        quint16 number;
        *(tcpMessage.dStream) >> number >> whichRate;
        sendGpioRates(number, whichRate);
    } else if (msgID == TCP_MSG_KEY::MSG_GPIO_RATE_RESET) {
        clearRates();
    } else if (msgID == TCP_MSG_KEY::MSG_DAC_REQUEST) {
        quint8 channel;
        *(tcpMessage.dStream) >> channel;
        MCP4728::DacChannel channelData;
        if (!dac_p || !dac_p->probeDevicePresence())
            return;
        dynamic_cast<MCP4728*>(dac_p.get())->readChannel(channel, channelData);
        float voltage = MCP4728::code2voltage(channelData);
        sendDacReadbackValue(channel, voltage);
    } else if (msgID == TCP_MSG_KEY::MSG_ADC_SAMPLE_REQUEST) {
        quint8 channel;
        *(tcpMessage.dStream) >> channel;
        sampleAdcEvent(channel);
    } else if (msgID == TCP_MSG_KEY::MSG_TEMPERATURE_REQUEST) {
        getTemperature();
    } else if (msgID == TCP_MSG_KEY::MSG_I2C_STATS_REQUEST) {
        sendI2cStats();
    } else if (msgID == TCP_MSG_KEY::MSG_I2C_SCAN_BUS) {
        scanI2cBus();
        sendI2cStats();
    } else if (msgID == TCP_MSG_KEY::MSG_SPI_STATS_REQUEST) {
        sendSpiStats();
    } else if (msgID == TCP_MSG_KEY::MSG_CALIB_REQUEST) {
        sendCalib();
    } else if (msgID == TCP_MSG_KEY::MSG_CALIB_SAVE) {
        if (calib != nullptr)
            calib->writeToEeprom();
        sendCalib();
    } else if (msgID == TCP_MSG_KEY::MSG_CALIB_SET) {
        std::vector<CalibStruct> calibs;
        quint8 nrEntries = 0;
        *(tcpMessage.dStream) >> nrEntries;
        for (int i = 0; i < nrEntries; i++) {
            CalibStruct item;
            *(tcpMessage.dStream) >> item;
            calibs.push_back(item);
        }
        receivedCalibItems(calibs);
    } else if (msgID == TCP_MSG_KEY::MSG_UBX_GNSS_CONFIG) {
        std::vector<GnssConfigStruct> gnss_configs;
        int nrEntries = 0;
        *(tcpMessage.dStream) >> nrEntries;
        for (int i = 0; i < nrEntries; i++) {
            GnssConfigStruct gnss_config;
            *(tcpMessage.dStream) >> gnss_config.gnssId >> gnss_config.resTrkCh >> gnss_config.maxTrkCh >> gnss_config.flags;
            gnss_configs.push_back(gnss_config);
        }
        emit setGnssConfig(gnss_configs);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        emit sendPollUbxMsg(UBX_MSG::CFG_GNSS);
    } else if (msgID == TCP_MSG_KEY::MSG_UBX_CFG_TP5) {
        UbxTimePulseStruct tp;
        *(tcpMessage.dStream) >> tp;
        emit UBXSetCfgTP5(tp);
        emit sendPollUbxMsg(UBX_MSG::CFG_TP5);
    } else if (msgID == TCP_MSG_KEY::MSG_UBX_CFG_SAVE) {
        emit UBXSaveCfg();
        emit sendPollUbxMsg(UBX_MSG::CFG_TP5);
        emit sendPollUbxMsg(UBX_MSG::CFG_GNSS);
    } else if (msgID == TCP_MSG_KEY::MSG_QUIT_CONNECTION) {
        QString closeAddress;
        *(tcpMessage.dStream) >> closeAddress;
        emit closeConnection(closeAddress);
    } else if (msgID == TCP_MSG_KEY::MSG_DAC_EEPROM_SET) {
        saveDacValuesToEeprom();
    } else if (msgID == TCP_MSG_KEY::MSG_HISTOGRAM_CLEAR) {
        QString histoName;
        *(tcpMessage.dStream) >> histoName;
        clearHisto(histoName);
    } else if (msgID == TCP_MSG_KEY::MSG_ADC_MODE_REQUEST) {
        TcpMessage answer(TCP_MSG_KEY::MSG_ADC_MODE);
        *(answer.dStream) << static_cast<quint8>(adcSamplingMode);
        emit sendTcpMessage(answer);
    } else if (msgID == TCP_MSG_KEY::MSG_ADC_MODE) {
        quint8 mode { 0 };
        *(tcpMessage.dStream) >> mode;
        setAdcSamplingMode(static_cast<ADC_SAMPLING_MODE>(mode));
        TcpMessage answer(TCP_MSG_KEY::MSG_ADC_MODE);
        *(answer.dStream) << static_cast<quint8>(adcSamplingMode);
        emit sendTcpMessage(answer);
    } else if (msgID == TCP_MSG_KEY::MSG_LOG_INFO) {
        sendLogInfo();
    } else if (msgID == TCP_MSG_KEY::MSG_GPIO_INHIBIT) {
        bool inhibit = true;
        *(tcpMessage.dStream) >> inhibit;
        if (pigHandler != nullptr) {
            pigHandler->setInhibited(inhibit);
            TcpMessage answer(TCP_MSG_KEY::MSG_GPIO_INHIBIT);
            *(answer.dStream) << pigHandler->isInhibited();
            emit sendTcpMessage(answer);
        }
    } else if (msgID == TCP_MSG_KEY::MSG_MQTT_INHIBIT) {
        bool inhibit { false };
        *(tcpMessage.dStream) >> inhibit;
        if (mqttHandler != nullptr) {
            mqttHandler->setInhibited(inhibit);
            TcpMessage answer(TCP_MSG_KEY::MSG_MQTT_INHIBIT);
            *(answer.dStream) << mqttHandler->isInhibited();
            emit sendTcpMessage(answer);
            emit requestMqttConnectionStatus();
        }
    } else if (msgID == TCP_MSG_KEY::MSG_VERSION) {
        sendVersionInfo();
    } else {
        qDebug() << "received unknown TCP message: msgID =" << QString::number(static_cast<int>(msgID));
    }
}

void Daemon::setAdcSamplingMode(ADC_SAMPLING_MODE mode)
{
    adcSamplingMode = mode;
    if (mode == ADC_SAMPLING_MODE::TRACE)
        samplingTimer.start();
    else
        samplingTimer.stop();
}

void Daemon::scanI2cBus()
{
    for (uint8_t addr = 0x04; addr <= 0x78; addr++) {
        bool alreadyThere = false;
        for (uint8_t i = 0; i < i2cDevice::getGlobalDeviceList().size(); i++) {
            if (addr == i2cDevice::getGlobalDeviceList()[i]->getAddress()) {
                alreadyThere = true;
                break;
            }
        }
        if (alreadyThere) {
            continue;
        }

        i2cDevice* new_dev { instantiateI2cDevice(addr) };
        if (new_dev && !new_dev->devicePresent())
            delete new_dev;
    }
}

void Daemon::sendLogInfo()
{
    LogInfoStruct lis { fileHandler->getInfo() };
    lis.logEnabled = config.storeLocal;
    TcpMessage answer(TCP_MSG_KEY::MSG_LOG_INFO);
    *(answer.dStream) << lis;
    emit sendTcpMessage(answer);
}

void Daemon::sendI2cStats()
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_I2C_STATS);
    quint8 nrDevices = i2cDevice::getGlobalDeviceList().size();
    quint32 bytesRead = i2cDevice::getGlobalNrBytesRead();
    quint32 bytesWritten = i2cDevice::getGlobalNrBytesWritten();
    *(tcpMessage.dStream) << nrDevices << bytesRead << bytesWritten;

    for (uint8_t i = 0; i < i2cDevice::getGlobalDeviceList().size(); i++) {
        uint8_t addr = i2cDevice::getGlobalDeviceList()[i]->getAddress();
        QString title = QString::fromStdString(i2cDevice::getGlobalDeviceList()[i]->getTitle());
        i2cDevice::getGlobalDeviceList()[i]->devicePresent();
        uint8_t status = i2cDevice::getGlobalDeviceList()[i]->getStatus();
        *(tcpMessage.dStream) << addr << title << status;
    }
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendSpiStats()
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_SPI_STATS);
    *(tcpMessage.dStream) << spiDevicePresent;
    emit sendTcpMessage(spiDevicePresent);
}

void Daemon::sendCalib()
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_CALIB_SET);
    bool valid = calib->isValid();
    bool eepValid = calib->isEepromValid();
    quint16 nrPars = calib->getCalibList().size();
    quint64 id = calib->getSerialID();
    *(tcpMessage.dStream) << valid << eepValid << id << nrPars;
    for (int i = 0; i < nrPars; i++) {
        *(tcpMessage.dStream) << calib->getCalibItem(i);
    }
    emit sendTcpMessage(tcpMessage);
}

void Daemon::receivedCalibItems(const std::vector<CalibStruct>& newCalibs)
{
    for (unsigned int i = 0; i < newCalibs.size(); i++) {
        calib->setCalibItem(newCalibs[i].name, newCalibs[i]);
    }
}

void Daemon::onGpsPropertyUpdatedGeodeticPos(const GeodeticPos& pos)
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_GEO_POS);
    (*tcpMessage.dStream) << pos.iTOW << pos.lon << pos.lat
                          << pos.height << pos.hMSL << pos.hAcc
                          << pos.vAcc;
    emit sendTcpMessage(tcpMessage);

    QString geohash = GeoHash::hashFromCoordinates(1e-7 * pos.lon, 1e-7 * pos.lat, 10);

    emit logParameter(LogParameter("geoLongitude", QString::number(1e-7 * pos.lon, 'f', 7) + " deg", LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("geoLatitude", QString::number(1e-7 * pos.lat, 'f', 7) + " deg", LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("geoHash", geohash + " ", LogParameter::LOG_LATEST));
    emit logParameter(LogParameter("geoHeightMSL", QString::number(1e-3 * pos.hMSL, 'f', 2) + " m", LogParameter::LOG_AVERAGE));
    if (histoMap.find("geoHeight") != histoMap.end())
        emit logParameter(LogParameter("meanGeoHeightMSL", QString::number(histoMap["geoHeight"].getMean(), 'f', 2) + " m", LogParameter::LOG_LATEST));
    emit logParameter(LogParameter("geoHorAccuracy", QString::number(1e-3 * pos.hAcc, 'f', 2) + " m", LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("geoVertAccuracy", QString::number(1e-3 * pos.vAcc, 'f', 2) + " m", LogParameter::LOG_AVERAGE));

    if (1e-3 * pos.vAcc < 100.) {
        QString name = "geoHeight";
        if (histoMap.find(name) != histoMap.end()) {
            histoMap[name].fill(1e-3 * pos.hMSL);
            if (currentDOP.vDOP > 0) {
                name = "weightedGeoHeight";
                double heightWeight = 100. / currentDOP.vDOP;
                histoMap[name].fill(1e-3 * pos.hMSL, heightWeight);
            }
        }
    }
    if (1e-3 * pos.hAcc < 100.) {
        QString name = "geoLongitude";
        histoMap[name].fill(1e-7 * pos.lon);
        name = "geoLatitude";
        histoMap[name].fill(1e-7 * pos.lat);
    }
}

void Daemon::sendHistogram(const Histogram& hist)
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_HISTOGRAM);
    (*tcpMessage.dStream) << hist;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendUbxMsgRates()
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_UBX_MSG_RATE);
    *(tcpMessage.dStream) << msgRateCfgs;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendDacReadbackValue(uint8_t channel, float voltage)
{
    if (channel > 3) {
        return;
    }

    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_DAC_READBACK);
    *(tcpMessage.dStream) << (quint8)channel << voltage;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendGpioPinEvent(uint8_t gpio_pin)
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_GPIO_EVENT);
    // reverse lookup of gpio function from given pin (first occurence)
    auto result = std::find_if(GPIO_PINMAP.begin(), GPIO_PINMAP.end(), [&gpio_pin](const std::pair<GPIO_SIGNAL, unsigned int>& item) { return item.second == gpio_pin; });
    if (result != GPIO_PINMAP.end()) {
        *(tcpMessage.dStream) << (GPIO_SIGNAL)result->first;
        emit sendTcpMessage(tcpMessage);
    }
}

void Daemon::sendBiasStatus()
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_BIAS_SWITCH);
    *(tcpMessage.dStream) << config.bias_ON;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendGainSwitchStatus()
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_GAIN_SWITCH);
    *(tcpMessage.dStream) << config.hi_gain;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendPreampStatus(uint8_t channel)
{
    if (channel > 1) {
        return;
    }
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_PREAMP_SWITCH);
    *(tcpMessage.dStream) << (quint8)channel << config.preamp_enable[channel];
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendPolarityStatus()
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_POLARITY_SWITCH);
    *(tcpMessage.dStream) << config.polarity[0] << config.polarity[1];
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendPcaChannel()
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_PCA_SWITCH);
    *(tcpMessage.dStream) << (quint8)config.pcaPortMask;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendVersionInfo()
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_VERSION);
    *(tcpMessage.dStream) << MuonPi::Version::hardware << MuonPi::Version::software;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendEventTriggerSelection()
{
    if (pigHandler == nullptr)
        return;
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_EVENTTRIGGER);
    *(tcpMessage.dStream) << (GPIO_SIGNAL)pigHandler->samplingTriggerSignal;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::sendExtendedMqttStatus(MuonPi::MqttHandler::Status status)
{
    bool bStatus { (status == MuonPi::MqttHandler::Status::Connected) };
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_MQTT_STATUS);
    *(tcpMessage.dStream) << bStatus << static_cast<int>(status);
    if (bStatus != mqttConnectionStatus) {
        if (bStatus) {
            qDebug() << "MQTT (re)connected";
            emit message(std::make_shared<gpio_state>(GPIO_PINMAP[STATUS1], 1));
        } else {
            qDebug() << "MQTT connection lost";
            emit message(std::make_shared<gpio_state>(GPIO_PINMAP[STATUS1], 0));
        }
    }
    mqttConnectionStatus = bStatus;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::rateCounterIntervalActualisation()
{
    if (xorCounts.isEmpty()) {
        xorCounts.push_back(0);
    }
    if (andCounts.isEmpty()) {
        andCounts.push_back(0);
    }
    timespec now;
    timespec_get(&now, TIME_UTC);
    int64_t diff = msecdiff(now, lastRateInterval);
    while (diff > 1000) {
        xorCounts.push_back(0);
        andCounts.push_back(0);
        while ((quint32)xorCounts.size() > (rateBufferTime)) {
            xorCounts.pop_front();
        }
        while ((quint32)andCounts.size() > (rateBufferTime)) {
            andCounts.pop_front();
        }
        lastRateInterval.tv_sec += 1;
        diff = msecdiff(now, lastRateInterval);
    }
}

void Daemon::clearRates()
{
    xorCounts.clear();
    xorCounts.push_back(0);
    andCounts.clear();
    andCounts.push_back(0);
    xorRatePoints.clear();
    andRatePoints.clear();
}

qreal Daemon::getRateFromCounts(quint8 which_rate)
{
    rateCounterIntervalActualisation();
    QList<quint64>* counts;
    if (which_rate == XOR_RATE) {
        counts = &xorCounts;
    } else if (which_rate == AND_RATE) {
        counts = &andCounts;
    } else {
        return -1.0;
    }
    quint64 sum = 0;
    for (auto count : *counts) {
        sum += count;
    }
    timespec now;
    timespec_get(&now, TIME_UTC);
    qreal timeInterval = ((qreal)(1000 * (counts->size() - 1))) + (qreal)msecdiff(now, lastRateInterval); // in ms
    qreal rate = sum / timeInterval * 1000;
    return (rate);
}

void Daemon::onRateBufferReminder()
{
    qreal secsSinceStart = 0.001 * (qreal)msecdiff(lastRateInterval, startOfProgram);
    qreal xorRate = getRateFromCounts(XOR_RATE);
    qreal andRate = getRateFromCounts(AND_RATE);
    QPointF xorPoint(secsSinceStart, xorRate);
    QPointF andPoint(secsSinceStart, andRate);
    xorRatePoints.append(xorPoint);
    andRatePoints.append(andPoint);
    emit logParameter(LogParameter("rateXOR", QString::number(xorRate) + " Hz", LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("rateAND", QString::number(andRate) + " Hz", LogParameter::LOG_AVERAGE));
    while ((quint32)xorRatePoints.size() > rateMaxShowInterval / rateBufferInterval) {
        xorRatePoints.pop_front();
    }
    while ((quint32)andRatePoints.size() > rateMaxShowInterval / rateBufferInterval) {
        andRatePoints.pop_front();
    }
}

void Daemon::sendGpioRates(int number, quint8 whichRate)
{
    if (pigHandler == nullptr) {
        return;
    }
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_GPIO_RATE);
    QVector<QPointF>* ratePoints;
    if (whichRate == XOR_RATE) {
        ratePoints = &xorRatePoints;
    } else if (whichRate == AND_RATE) {
        ratePoints = &andRatePoints;
    } else {
        return;
    }
    QVector<QPointF> someRates;
    if (number >= ratePoints->size() || number == 0) {
        number = ratePoints->size() - 1;
    }
    if (!ratePoints->isEmpty()) {
        for (int i = 0; i < number; i++) {
            someRates.push_front(ratePoints->at(ratePoints->size() - 1 - i));
        }
    }
    *(tcpMessage.dStream) << whichRate << someRates;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::onAdcSampleReady(ADS1115::Sample sample)
{
    const uint8_t channel = sample.channel;
    float voltage = sample.voltage;
    if (channel != 0) {
        TcpMessage tcpMessage(TCP_MSG_KEY::MSG_ADC_SAMPLE);
        *(tcpMessage.dStream) << (quint8)channel << voltage;
        emit sendTcpMessage(tcpMessage);
    } else {
        if (adcSamplingMode == ADC_SAMPLING_MODE::TRACE) {
            adcSamplesBuffer.push_back(voltage);
            if (adcSamplesBuffer.size() > Config::Hardware::ADC::buffer_size)
                adcSamplesBuffer.pop_front();
            if (currentAdcSampleIndex == 0) {
                TcpMessage tcpMessage(TCP_MSG_KEY::MSG_ADC_SAMPLE);
                *(tcpMessage.dStream) << (quint8)channel << voltage;
                emit sendTcpMessage(tcpMessage);
                histoMap["pulseHeight"].fill(voltage);
            }
            if (currentAdcSampleIndex >= 0) {
                currentAdcSampleIndex++;
                if (currentAdcSampleIndex >= (Config::Hardware::ADC::buffer_size - Config::Hardware::ADC::pretrigger)) {
                    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_ADC_TRACE);
                    *(tcpMessage.dStream) << (quint16)adcSamplesBuffer.size();
                    for (int i = 0; i < adcSamplesBuffer.size(); i++) {
                        *(tcpMessage.dStream) << adcSamplesBuffer[i];
                    }
                    emit sendTcpMessage(tcpMessage);
                    currentAdcSampleIndex = -1;
                }
            }
        } else if (adcSamplingMode == ADC_SAMPLING_MODE::PEAK) {
            TcpMessage tcpMessage(TCP_MSG_KEY::MSG_ADC_SAMPLE);
            *(tcpMessage.dStream) << (quint8)channel << voltage;
            emit sendTcpMessage(tcpMessage);
            histoMap["pulseHeight"].fill(voltage);
            currentAdcSampleIndex = 0;
        }
    }
    if (adc_p) {
        emit logParameter(LogParameter("adcSamplingTime", QString::number(adc_p->getLastConvTime()) + " ms", LogParameter::LOG_AVERAGE));
        histoMap["adcSampleTime"].fill(adc_p->getLastConvTime());
    }
}

void Daemon::sampleAdcEvent(uint8_t channel)
{
    if (adc_p == nullptr || adcSamplingMode == ADC_SAMPLING_MODE::DISABLED) {
        return;
    }
    if (std::dynamic_pointer_cast<ADS1115>(adc_p)->getStatus() & i2cDevice::MODE_UNREACHABLE) {
        return;
    }
    //adc->setActiveChannel( channel );
    adc_p->triggerConversion(channel);
}

void Daemon::sampleAdc0Event()
{
    sampleAdcEvent(0);
    currentAdcSampleIndex = 0;
}

void Daemon::sampleAdc0TraceEvent()
{
    sampleAdcEvent(0);
}

void Daemon::getTemperature()
{
    if (!temp_sensor_p) {
        return;
    }
    if (dynamic_cast<i2cDevice*>(temp_sensor_p.get())->getStatus() & i2cDevice::MODE_UNREACHABLE) {
        return;
    }
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_TEMPERATURE);
    if (temp_sensor_p->getName() == "MIC184" && dynamic_cast<i2cDevice*>(temp_sensor_p.get())->getAddress() < 0x4c) {
        // the attached temp sensor has a remote zone
        if (dynamic_cast<MIC184*>(temp_sensor_p.get())->isExternal())
            return;
    }
    float value = temp_sensor_p->getTemperature();
    *(tcpMessage.dStream) << value;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::setEventTriggerSelection(GPIO_SIGNAL signal)
{
    if (pigHandler == nullptr)
        return;
    auto it = GPIO_PINMAP.find(signal);
    if (it == GPIO_PINMAP.end())
        return;

    if (verbose > 0) {
        qInfo() << "changed event selection to signal" << (unsigned int)signal;
    }
    emit setSamplingTriggerSignal(signal);
    emit logParameter(LogParameter("gpioTriggerSelection", "0x" + QString::number((int)pigHandler->samplingTriggerSignal, 16), LogParameter::LOG_EVERY));
}

// ALL FUNCTIONS ABOUT SETTINGS FOR THE I2C-DEVICES (DAC, ADC, PCA...)
void Daemon::setPcaChannel(uint8_t channel)
{
    if (!io_extender_p || !io_extender_p->probeDevicePresence()) {
        return;
    }
    if (channel > ((MuonPi::Version::hardware.major == 1) ? 3 : 7)) {
        qWarning() << "invalid PCA channel selection: ch" << (int)channel << "...ignoring";
        return;
    }
    if (verbose > 0) {
        qInfo() << "changed pcaPortMask to" << channel;
    }
    config.pcaPortMask = channel;
    io_extender_p->setOutputState(channel);
    emit logParameter(LogParameter("ubxInputSwitch", "0x" + QString::number(config.pcaPortMask, 16), LogParameter::LOG_EVERY));
}

void Daemon::setBiasVoltage(float voltage)
{
    if (verbose > 0) {
        qInfo() << "change biasVoltage to" << voltage;
    }
    if (dac_p && dac_p->probeDevicePresence()) {
        dac_p->setVoltage(Config::Hardware::DAC::Channel::bias, voltage);
        emit logParameter(LogParameter("biasDAC", QString::number(voltage) + " V", LogParameter::LOG_EVERY));
        config.biasVoltage = voltage;
    }
    clearRates();
}

void Daemon::setBiasStatus(bool status)
{
    config.bias_ON = status;
    if (verbose > 0) {
        qInfo() << "change biasStatus to" << status;
    }

    if (status) {
        emit message(std::make_shared<gpio_state>(GPIO_PINMAP[UBIAS_EN], (MuonPi::Version::hardware.major == 1) ? 1 : 0));
    } else {
        emit message(std::make_shared<gpio_state>(GPIO_PINMAP[UBIAS_EN], (MuonPi::Version::hardware.major == 1) ? 0 : 1));
    }
    emit logParameter(LogParameter("biasSwitch", QString::number(status), LogParameter::LOG_EVERY));
    clearRates();
}

void Daemon::setDacThresh(uint8_t channel, float threshold)
{
    if (threshold < 0 || channel > 3) {
        return;
    }
    if (channel == 2 || channel == 3) {
        if (dac_p) {
            dac_p->setVoltage(channel, threshold);
        }
        return;
    }
    if (threshold > 4.095) {
        threshold = 4.095;
    }
    if (verbose > 0) {
        qInfo() << "change dacThresh" << channel << "to" << threshold;
    }
    config.thresholdVoltage[channel] = threshold;
    clearRates();
    if (dac_p && dac_p->setVoltage(channel, threshold)) {
        emit logParameter(LogParameter("thresh" + QString::number(channel + 1), QString::number(config.thresholdVoltage[channel]) + " V", LogParameter::LOG_EVERY));
    }
}

void Daemon::saveDacValuesToEeprom()
{
    if (!dac_p || !dac_p->probeDevicePresence())
        return;
    bool ok = dac_p->storeSettings();
    if (!ok)
        std::cerr << "error writing DAC eeprom" << std::endl;
}

bool Daemon::readEeprom()
{
    if (!eep_p || !eep_p->probeDevicePresence()) {
        return false;
    }
    uint16_t n = 256;
    uint8_t buf[256];
    for (int i = 0; i < n; i++)
        buf[i] = 0;
    bool retval = (eep_p->readBytes(0, n, buf) == n);
    std::cout << "*** EEPROM content ***" << std::endl;
    for (int j = 0; j < 16; j++) {
        std::cout << std::hex << std::setfill('0') << std::setw(2) << j * 16 << ": ";
        for (int i = 0; i < 16; i++) {
            std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)buf[j * 16 + i] << " ";
        }
        std::cout << std::endl;
    }
    return retval;
}

void Daemon::setUbxMsgRates(QMap<uint16_t, int>& ubxMsgRates)
{
    for (QMap<uint16_t, int>::iterator it = ubxMsgRates.begin(); it != ubxMsgRates.end(); it++) {
        emit UBXSetCfgMsgRate(it.key(), 1, it.value());
        emit sendPollUbxMsgRate(it.key());
        waitingForAppliedMsgRate++;
    }
}

// ALL FUNCTIONS ABOUT UBLOX GPS MODULE
void Daemon::configGps()
{
    // set up ubx as only outPortProtocol
    emit UBXSetCfgPrt(1, PROTO_UBX);

    // set dynamic model: Stationary
    qDebug() << "setting GNSS dynamic model to" << config.gnss_dynamic_model;
    emit UBXSetDynModel(config.gnss_dynamic_model);

    emit UBXSetAopCfg(true);

    emit sendPollUbxMsg(UBX_MSG::MON_VER);

    int measrate = 10;
    emit UBXSetCfgRate(1000 / measrate, 1); // UBX_RATE

    emit UBXSetCfgMsgRate(UBX_MSG::TIM_TM2, 1, 1); // TIM-TM2
    emit UBXSetCfgMsgRate(UBX_MSG::TIM_TP, 1, 0); // TIM-TP
    emit UBXSetCfgMsgRate(UBX_MSG::NAV_TIMEUTC, 1, 131); // NAV-TIMEUTC
    emit UBXSetCfgMsgRate(UBX_MSG::MON_HW, 1, 47); // MON-HW
    emit UBXSetCfgMsgRate(UBX_MSG::MON_HW2, 1, 49); // MON-HW
    emit UBXSetCfgMsgRate(UBX_MSG::NAV_POSLLH, 1, 127); // MON-POSLLH
    // probably also configured with UBX-CFG-INFO...
    emit UBXSetCfgMsgRate(UBX_MSG::NAV_TIMEGPS, 1, 0); // NAV-TIMEGPS
    emit UBXSetCfgMsgRate(UBX_MSG::NAV_SOL, 1, 0); // NAV-SOL
    emit UBXSetCfgMsgRate(UBX_MSG::NAV_STATUS, 1, 71); // NAV-STATUS
    emit UBXSetCfgMsgRate(UBX_MSG::NAV_CLOCK, 1, 189); // NAV-CLOCK
    emit UBXSetCfgMsgRate(UBX_MSG::MON_RXBUF, 1, 53); // MON-TXBUF
    emit UBXSetCfgMsgRate(UBX_MSG::MON_TXBUF, 1, 51); // MON-TXBUF
    emit UBXSetCfgMsgRate(UBX_MSG::NAV_SBAS, 1, 0); // NAV-SBAS
    emit UBXSetCfgMsgRate(UBX_MSG::NAV_DOP, 1, 254); // NAV-DOP
    // this poll is for checking the port cfg (which protocols are enabled etc.)
    emit sendPollUbxMsg(UBX_MSG::CFG_PRT);
    emit sendPollUbxMsg(UBX_MSG::MON_VER);
    emit sendPollUbxMsg(UBX_MSG::MON_VER);
    emit sendPollUbxMsg(UBX_MSG::MON_VER);
    emit sendPollUbxMsg(UBX_MSG::CFG_GNSS);
    emit sendPollUbxMsg(UBX_MSG::CFG_NAVX5);
    emit sendPollUbxMsg(UBX_MSG::CFG_ANT);
    emit sendPollUbxMsg(UBX_MSG::CFG_TP5);

    configGpsForVersion();
}

void Daemon::configGpsForVersion()
{
    if (QtSerialUblox::getProtVersion() <= 0.1)
        return;
    if (QtSerialUblox::getProtVersion() > 15.0) {
        if (std::find(allMsgCfgID.begin(), allMsgCfgID.end(), UBX_MSG::NAV_SAT) == allMsgCfgID.end()) {
            allMsgCfgID.push_back(UBX_MSG::NAV_SAT);
        }
        emit UBXSetCfgMsgRate(UBX_MSG::NAV_SAT, 1, 69); // NAV-SAT
        emit UBXSetCfgMsgRate(UBX_MSG::NAV_SVINFO, 1, 0);
    } else
        emit UBXSetCfgMsgRate(UBX_MSG::NAV_SVINFO, 1, 69); // NAV-SVINFO
}

void Daemon::pollAllUbxMsgRate()
{
    for (const auto& elem : allMsgCfgID) {
        emit sendPollUbxMsgRate(elem);
    }
}

void Daemon::UBXReceivedAckNak(uint16_t ackedMsgID, uint16_t ackedCfgMsgID)
{
    // the value was already set correctly before by either poll or set,
    // if not acknowledged or timeout we set the value to -1 (unknown/undefined)
    switch (ackedMsgID) {
    case UBX_MSG::CFG_MSG:
        msgRateCfgs.insert(ackedCfgMsgID, -1);
        break;
    default:
        break;
    }
}

void Daemon::UBXReceivedMsgRateCfg(uint16_t msgID, uint8_t rate)
{
    msgRateCfgs.insert(msgID, rate);
    if (verbose > 2)
        qDebug() << "received msg rate: id=" << QString::number(msgID, 16) << " rate=" << (int)rate;
    waitingForAppliedMsgRate--;
    if (waitingForAppliedMsgRate < 0) {
        waitingForAppliedMsgRate = 0;
    }
    if (waitingForAppliedMsgRate == 0) {
        sendUbxMsgRates();
    }
}
void Daemon::onGpsMonHWUpdated(const GnssMonHwStruct& hw)
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_UBX_MONHW);
    (*tcpMessage.dStream) << hw;
    emit sendTcpMessage(tcpMessage);
    emit logParameter(LogParameter("preampNoise", QString::number(-hw.noise) + " dBHz", LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("preampAGC", QString::number(hw.agc), LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("antennaStatus", QString::number(hw.antStatus), LogParameter::LOG_LATEST));
    emit logParameter(LogParameter("antennaPower", QString::number(hw.antPower), LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("jammingLevel", QString::number(hw.jamInd / 2.55, 'f', 1) + " %", LogParameter::LOG_AVERAGE));
}

void Daemon::onGpsMonHW2Updated(const GnssMonHw2Struct& hw2)
{
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_UBX_MONHW2);
    (*tcpMessage.dStream) << hw2;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::onGpsPropertyUpdatedGnss(const std::vector<GnssSatellite>& sats,
    std::chrono::duration<double> lastUpdated)
{
    vector<GnssSatellite> visibleSats = sats;
    std::sort(visibleSats.begin(), visibleSats.end(), GnssSatellite::sortByCnr);
    while (visibleSats.size() > 0 && visibleSats.back().getCnr() == 0)
        visibleSats.pop_back();

    if (verbose > 3) {
        std::cout << std::chrono::system_clock::now()
                - std::chrono::duration_cast<std::chrono::microseconds>(lastUpdated)
                  << "Nr of satellites: " << visibleSats.size() << " (out of " << sats.size() << std::endl;
        // read nrSats property without evaluation to prevent separate display of this property
        // in the common message poll below
        GnssSatellite::PrintHeader(true);
        for (unsigned int i = 0; i < sats.size(); i++) {
            sats[i].Print(i, false);
        }
    }
    int N = sats.size();
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_GNSS_SATS);
    (*tcpMessage.dStream) << N;
    for (int i = 0; i < N; i++) {
        (*tcpMessage.dStream) << sats[i];
    }
    emit sendTcpMessage(tcpMessage);
    nrSats = Property("nrSats", N);
    nrVisibleSats = QVariant { static_cast<qulonglong>(visibleSats.size()) };

    propertyMap["nrSats"] = Property("nrSats", N);
    propertyMap["visSats"] = Property("visSats", visibleSats.size());

    int usedSats = 0, maxCnr = 0;
    if (visibleSats.size()) {
        for (auto sat : visibleSats) {
            if (sat.fUsed)
                usedSats++;
            if (sat.fCnr > maxCnr)
                maxCnr = sat.fCnr;
        }
    }
    propertyMap["usedSats"] = Property("usedSats", usedSats);
    propertyMap["maxCNR"] = Property("maxCNR", maxCnr);

    emit logParameter(LogParameter("sats", QString("%1").arg(visibleSats.size()), LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("usedSats", QString("%1").arg(usedSats), LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("maxCNR", QString("%1").arg(maxCnr) + " dB", LogParameter::LOG_AVERAGE));
}

void Daemon::onUBXReceivedGnssConfig(uint8_t numTrkCh, const std::vector<GnssConfigStruct>& gnssConfigs)
{
    if (verbose > 3) {
        // put some verbose output here
    }
    int N = gnssConfigs.size();
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_UBX_GNSS_CONFIG);
    (*tcpMessage.dStream) << (int)numTrkCh << N;
    for (int i = 0; i < N; i++) {
        (*tcpMessage.dStream) << gnssConfigs[i].gnssId << gnssConfigs[i].resTrkCh << gnssConfigs[i].maxTrkCh << gnssConfigs[i].flags;
    }
    emit sendTcpMessage(tcpMessage);
}

void Daemon::onUBXReceivedTP5(const UbxTimePulseStruct& tp)
{
    static uint8_t forceUtcSetCounter = 0;
    if (verbose > 3) {
        // put some verbose output here
    }
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_UBX_CFG_TP5);
    (*tcpMessage.dStream) << tp;
    emit sendTcpMessage(tcpMessage);
    // check here if UTC is selected as time source
    // this should probably be implemented somewhere else, maybe at ublox init
    // however, for the timestamping to work correctly, setting the time grid to UTC is mandatory!
    int timeGrid = (tp.flags & UbxTimePulseStruct::GRID_UTC_GPS) >> 7;
    if (timeGrid != 0 && forceUtcSetCounter++ < 3) {
        UbxTimePulseStruct newTp = tp;
        newTp.flags &= ~((uint32_t)UbxTimePulseStruct::GRID_UTC_GPS);
        emit UBXSetCfgTP5(newTp);
        qDebug() << "forced time grid to UTC";
        emit sendPollUbxMsg(UBX_MSG::CFG_TP5);
    }
}

void Daemon::onUBXReceivedTxBuf(uint8_t txUsage, uint8_t txPeakUsage)
{
    TcpMessage* tcpMessage;
    if (verbose > 3) {
        qDebug() << "TX buf usage:" << (int)txUsage << "%";
        qDebug() << "TX buf peak usage:" << (int)txPeakUsage << "%";
    }
    tcpMessage = new TcpMessage(TCP_MSG_KEY::MSG_UBX_TXBUF);
    *(tcpMessage->dStream) << (quint8)txUsage << (quint8)txPeakUsage;
    emit sendTcpMessage(*tcpMessage);
    delete tcpMessage;
    emit logParameter(LogParameter("TXBufUsage", QString::number(txUsage) + " %", LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("maxTXBufUsage", QString::number(txPeakUsage) + " %", LogParameter::LOG_LATEST));
}

void Daemon::onUBXReceivedRxBuf(uint8_t rxUsage, uint8_t rxPeakUsage)
{
    TcpMessage* tcpMessage;
    if (verbose > 3) {
        qDebug() << "RX buf usage:" << (int)rxUsage << "%";
        qDebug() << "RX buf peak usage:" << (int)rxPeakUsage << "%";
    }
    tcpMessage = new TcpMessage(TCP_MSG_KEY::MSG_UBX_RXBUF);
    *(tcpMessage->dStream) << (quint8)rxUsage << (quint8)rxPeakUsage;
    emit sendTcpMessage(*tcpMessage);
    delete tcpMessage;
    emit logParameter(LogParameter("RXBufUsage", QString::number(rxUsage) + " %", LogParameter::LOG_AVERAGE));
    emit logParameter(LogParameter("maxRXBufUsage", QString::number(rxPeakUsage) + " %", LogParameter::LOG_LATEST));
}

void Daemon::gpsPropertyUpdatedUint8(uint8_t data, std::chrono::duration<double> updateAge,
    char propertyName)
{
    TcpMessage* tcpMessage;
    switch (propertyName) {
    case 's':
        if (verbose > 3)
            std::cout << std::chrono::system_clock::now()
                    - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
                      << "Nr of available satellites: " << (int)data << std::endl;
        break;
    case 'e':
        if (verbose > 3)
            std::cout << std::chrono::system_clock::now()
                    - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
                      << "quant error: " << (int)data << " ps" << std::endl;
        break;
    case 'f':
        if (verbose > 3)
            std::cout << std::chrono::system_clock::now()
                    - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
                      << "Fix value: " << (int)data << std::endl;
        tcpMessage = new TcpMessage(TCP_MSG_KEY::MSG_UBX_FIXSTATUS);
        *(tcpMessage->dStream) << (quint8)data;
        emit sendTcpMessage(*tcpMessage);
        delete tcpMessage;
        emit logParameter(LogParameter("fixStatus", QString::number(data), LogParameter::LOG_LATEST));
        emit logParameter(LogParameter("fixStatusString", QString::fromLocal8Bit(Gnss::FixType::name[data]), LogParameter::LOG_LATEST));
        fixStatus = QVariant(data);
        propertyMap["fixStatus"] = Property("fixStatus", QString::fromLocal8Bit(Gnss::FixType::name[data]));
        break;
    default:
        break;
    }
}

void Daemon::gpsPropertyUpdatedUint32(uint32_t data, chrono::duration<double> updateAge,
    char propertyName)
{
    TcpMessage* tcpMessage;
    switch (propertyName) {
    case 'a':
        if (verbose > 3)
            std::cout << std::chrono::system_clock::now()
                    - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
                      << "time accuracy: " << data << " ns" << std::endl;
        tcpMessage = new TcpMessage(TCP_MSG_KEY::MSG_UBX_TIME_ACCURACY);
        *(tcpMessage->dStream) << (quint32)data;
        emit sendTcpMessage(*tcpMessage);
        delete tcpMessage;
        emit logParameter(LogParameter("timeAccuracy", QString::number(data) + " ns", LogParameter::LOG_AVERAGE));
        break;
    case 'f':
        if (verbose > 3)
            std::cout << std::chrono::system_clock::now()
                    - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
                      << "frequency accuracy: " << data << " ps/s" << std::endl;
        tcpMessage = new TcpMessage(TCP_MSG_KEY::MSG_UBX_FREQ_ACCURACY);
        *(tcpMessage->dStream) << (quint32)data;
        emit sendTcpMessage(*tcpMessage);
        delete tcpMessage;
        emit logParameter(LogParameter("freqAccuracy", QString::number(data) + " ps/s", LogParameter::LOG_AVERAGE));
        break;
    case 'u':
        if (verbose > 3)
            std::cout << std::chrono::system_clock::now()
                    - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
                      << "Ublox uptime: " << data << " s" << std::endl;
        tcpMessage = new TcpMessage(TCP_MSG_KEY::MSG_UBX_UPTIME);
        *(tcpMessage->dStream) << (quint32)data;
        emit sendTcpMessage(*tcpMessage);
        delete tcpMessage;
        emit logParameter(LogParameter("ubloxUptime", QString::number(data) + " s", LogParameter::LOG_LATEST));
        break;
    case 'c':
        if (verbose > 3)
            std::cout << std::chrono::system_clock::now()
                    - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
                      << "rising edge counter: " << data << std::endl;
        propertyMap["events"] = Property("events", (quint16)data);
        tcpMessage = new TcpMessage(TCP_MSG_KEY::MSG_UBX_EVENTCOUNTER);
        *(tcpMessage->dStream) << (quint32)data;
        emit sendTcpMessage(*tcpMessage);
        delete tcpMessage;
        break;
    default:
        break;
    }
}

void Daemon::gpsPropertyUpdatedInt32(int32_t data, std::chrono::duration<double> updateAge,
    char propertyName)
{
    switch (propertyName) {
    case 'd':
        if (verbose > 3)
            std::cout << std::chrono::system_clock::now()
                    - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
                      << "clock drift: " << data << " ns/s" << std::endl;
        logParameter(LogParameter("clockDrift", QString::number(data) + " ns/s", LogParameter::LOG_AVERAGE));
        propertyMap["clkDrift"] = Property("clkDrift", (qint32)data);
        break;
    case 'b':
        if (verbose > 3)
            std::cout << std::chrono::system_clock::now()
                    - std::chrono::duration_cast<std::chrono::microseconds>(updateAge)
                      << "clock bias: " << data << " ns" << std::endl;
        emit logParameter(LogParameter("clockBias", QString::number(data) + " ns", LogParameter::LOG_AVERAGE));
        propertyMap["clkBias"] = Property("clkBias", (qint32)data);
        break;
    default:
        break;
    }
}

void Daemon::UBXReceivedVersion(const QString& swString, const QString& hwString, const QString& protString)
{
    static bool initialVersionInfo = true;
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_UBX_VERSION);
    (*tcpMessage.dStream) << swString << hwString << protString;
    emit sendTcpMessage(tcpMessage);
    emit logParameter(LogParameter("UBX_SW_Version", swString, LogParameter::LOG_ONCE));
    emit logParameter(LogParameter("UBX_HW_Version", hwString, LogParameter::LOG_ONCE));
    emit logParameter(LogParameter("UBX_Prot_Version", protString, LogParameter::LOG_ONCE));
    if (initialVersionInfo) {
        configGpsForVersion();
        qInfo() << "Ublox version:" << hwString << "(fw:" << swString << "prot:" << protString << ")";
    }
    initialVersionInfo = false;
}

void Daemon::toConsole(const QString& data)
{
    std::cout << data << std::endl;
}

void Daemon::gpsToConsole(const QString& data)
{
    std::cout << data << std::flush;
}

void Daemon::gpsConnectionError()
{
}

// ALL OTHER UTITLITY FUNCTIONS
void Daemon::onMadeConnection(QString remotePeerAddress, quint16 remotePeerPort, QString /*localAddress*/, quint16 /*localPort*/)
{
    if (verbose > 3)
        std::cout << "established connection with " << remotePeerAddress << ":" << remotePeerPort << std::endl;

    emit sendPollUbxMsg(UBX_MSG::MON_VER);
    emit sendPollUbxMsg(UBX_MSG::CFG_GNSS);
    emit sendPollUbxMsg(UBX_MSG::CFG_NAV5);
    emit sendPollUbxMsg(UBX_MSG::CFG_TP5);
    emit sendPollUbxMsg(UBX_MSG::CFG_NAVX5);

    sendVersionInfo();
    sendBiasStatus();
    emit message(std::make_shared<bias_voltage>(config.biasVoltage));
    emit message(std::make_shared<dac_threshold>(0, config.thresholdVoltage[0]));
    emit message(std::make_shared<dac_threshold>(1, config.thresholdVoltage[1]));
    sendPcaChannel();
    sendEventTriggerSelection();
}

void Daemon::onStoppedConnection(QString remotePeerAddress, quint16 remotePeerPort, QString /*localAddress*/, quint16 /*localPort*/,
    quint32 timeoutTime, quint32 connectionDuration)
{
    if (verbose > 3) {
        qDebug() << "stopped connection with" << remotePeerAddress << ":" << remotePeerPort;
        qDebug() << "connection timeout at" << timeoutTime << " connection lasted" << connectionDuration << "s";
    }
}

void Daemon::displayError(QString message)
{
    qDebug() << "Daemon:" << message;
}

void Daemon::displaySocketError(int socketError, QString message)
{
    switch (socketError) {
    case QAbstractSocket::HostNotFoundError:
        qCritical() << tr("The host was not found. Please check the host and port settings.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        qCritical() << tr("The connection was refused by the peer. "
                          "Make sure the server is running, "
                          "and check that the host name and port "
                          "settings are correct.");
        break;
    default:
        qCritical() << tr("The following error occurred: %1.").arg(message);
    }
    std::flush(std::cout);
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

    std::cout << secs.count() << "." << std::setw(6) << std::setfill('0') << subs.count() << " " << std::setfill(' ');
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
void Daemon::intSignalHandler(int)
{
    char a = 1;
    ::write(sigintFd[0], &a, sizeof(a));
}

void Daemon::aquireMonitoringParameters()
{
    if (temp_sensor_p && temp_sensor_p->probeDevicePresence()) {
        if (temp_sensor_p->getName() == "MIC184" && dynamic_cast<i2cDevice*>(temp_sensor_p.get())->getAddress() < 0x4c) {
            // the attached temp sensor has a remote zone
            // switch zones alternating
            bool is_ext { dynamic_cast<MIC184*>(temp_sensor_p.get())->isExternal() };
            if (is_ext) {
                emit logParameter(LogParameter("sensor_temperature", QString::number(temp_sensor_p->getTemperature()) + " degC", LogParameter::LOG_AVERAGE));
            } else {
                emit logParameter(LogParameter("temperature", QString::number(temp_sensor_p->getTemperature()) + " degC", LogParameter::LOG_AVERAGE));
            }
            dynamic_cast<MIC184*>(temp_sensor_p.get())->setExternal(!is_ext);
        } else {
            emit logParameter(LogParameter("temperature", QString::number(temp_sensor_p->getTemperature()) + " degC", LogParameter::LOG_AVERAGE));
        }
    }

    double v1 = 0., v2 = 0.;
    if (adc_p && (!(std::dynamic_pointer_cast<ADS1115>(adc_p)->getStatus() & i2cDevice::MODE_UNREACHABLE)) && (std::dynamic_pointer_cast<ADS1115>(adc_p)->getStatus() & (i2cDevice::MODE_NORMAL | i2cDevice::MODE_FORCE))) {
        v1 = adc_p->getVoltage(2);
        v2 = adc_p->getVoltage(3);
        if (calib && calib->getCalibItem("VDIV").name == "VDIV") {
            CalibStruct vdivItem = calib->getCalibItem("VDIV");
            std::istringstream istr(vdivItem.value);
            double vdiv;
            istr >> vdiv;
            vdiv /= 100.;
            logParameter(LogParameter("calib_vdiv", QString::number(vdiv), LogParameter::LOG_ONCE));
            istr.clear();
            istr.str(calib->getCalibItem("RSENSE").value);
            double rsense;
            istr >> rsense;
            if (verbose > 2) {
                qDebug() << "rsense:" << QString::fromStdString(calib->getCalibItem("RSENSE").value) << " (" << rsense << ")";
            }
            rsense /= 10. * 1000.; // yields Rsense in MOhm
            logParameter(LogParameter("calib_rsense", QString::number(rsense * 1000.) + " kOhm", LogParameter::LOG_ONCE));
            double ubias = v2 * vdiv;
            logParameter(LogParameter("vbias", QString::number(ubias) + " V", LogParameter::LOG_AVERAGE));
            histoMap["Bias Voltage"].fill(ubias);
            double usense = (v1 - v2) * vdiv;
            logParameter(LogParameter("vsense", QString::number(usense) + " V", LogParameter::LOG_AVERAGE));

            CalibStruct flagItem = calib->getCalibItem("CALIB_FLAGS");
            int calFlags = 0;

            istr.clear();
            istr.str(flagItem.value);
            istr >> calFlags;
            if (verbose > 2) {
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
                qDebug() << "cal flags:" << QString::fromStdString(flagItem.value) << " (" << (int)calFlags << dec << ")";
#else
                qDebug() << "cal flags:" << QString::fromStdString(flagItem.value) << " (" << (int)calFlags << Qt::dec << ")";
#endif
            }
            double icorr = 0.;
            if (calFlags & CalibStruct::CALIBFLAGS_CURRENT_COEFFS) {
                double islope, ioffs;
                istr.clear();
                istr.str(calib->getCalibItem("COEFF2").value);
                istr >> ioffs;
                logParameter(LogParameter("calib_coeff2", QString::number(ioffs), LogParameter::LOG_ONCE));
                istr.clear();
                istr.str(calib->getCalibItem("COEFF3").value);
                istr >> islope;
                logParameter(LogParameter("calib_coeff3", QString::number(islope), LogParameter::LOG_ONCE));
                icorr = ubias * islope + ioffs;
            }
            double ibias = usense / rsense - icorr;
            histoMap["Bias Current"].fill(ibias);
            logParameter(LogParameter("ibias", QString::number(ibias) + " uA", LogParameter::LOG_AVERAGE));

        } else {
            logParameter(LogParameter("vadc3", QString::number(v1) + " V", LogParameter::LOG_AVERAGE));
            logParameter(LogParameter("vadc4", QString::number(v2) + " V", LogParameter::LOG_AVERAGE));
        }
    }
}

void Daemon::onLogParameterPolled()
{
    // connect to the regular log timer signal to log several non-regularly polled parameters
    emit logParameter(LogParameter("biasSwitch", QString::number(config.bias_ON), LogParameter::LOG_ON_CHANGE));
    emit logParameter(LogParameter("preampSwitch1", QString::number((int)config.preamp_enable[0]), LogParameter::LOG_ON_CHANGE));
    emit logParameter(LogParameter("preampSwitch2", QString::number((int)config.preamp_enable[1]), LogParameter::LOG_ON_CHANGE));
    emit logParameter(LogParameter("gainSwitch", QString::number((int)config.hi_gain), LogParameter::LOG_ON_CHANGE));
    emit logParameter(LogParameter("polaritySwitch1", QString::number((int)config.polarity[0]), LogParameter::LOG_ON_CHANGE));
    emit logParameter(LogParameter("polaritySwitch2", QString::number((int)config.polarity[1]), LogParameter::LOG_ON_CHANGE));

    if (dac_p && dac_p->probeDevicePresence()) {
        emit logParameter(LogParameter("thresh1", QString::number(config.thresholdVoltage[0]) + " V", LogParameter::LOG_ON_CHANGE));
        emit logParameter(LogParameter("thresh2", QString::number(config.thresholdVoltage[1]) + " V", LogParameter::LOG_ON_CHANGE));
        emit logParameter(LogParameter("biasDAC", QString::number(config.biasVoltage) + " V", LogParameter::LOG_ON_CHANGE));
    }

    if (io_extender_p && io_extender_p->probeDevicePresence())
        emit logParameter(LogParameter("ubxInputSwitch", "0x" + QString::number(config.pcaPortMask, 16), LogParameter::LOG_ON_CHANGE));
    if (pigHandler != nullptr)
        emit logParameter(LogParameter("gpioTriggerSelection", "0x" + QString::number((int)pigHandler->samplingTriggerSignal, 16), LogParameter::LOG_ON_CHANGE));

    for (auto& hist : histoMap) {
        sendHistogram(hist);
        hist.rescale();
    }

    sendLogInfo();
    if (verbose > 2) {
        qDebug() << "current data file:" << fileHandler->dataFileInfo().absoluteFilePath();
        qDebug() << "file size: " << fileHandler->dataFileInfo().size() / (1024 * 1024) << "MiB";
    }

    // Since Linux 2.3.23 (i386) and Linux 2.3.48 (all architectures) the
    //        structure is:
    //
    //            struct sysinfo {
    //                long uptime;             /* Seconds since boot */
    //                unsigned long loads[3];  /* 1, 5, and 15 minute load averages */
    //                unsigned long totalram;  /* Total usable main memory size */
    //                unsigned long freeram;   /* Available memory size */
    //                unsigned long sharedram; /* Amount of shared memory */
    //                unsigned long bufferram; /* Memory used by buffers */
    //                unsigned long totalswap; /* Total swap space size */
    //                unsigned long freeswap;  /* Swap space still available */
    //                unsigned short procs;    /* Number of current processes */
    //                unsigned long totalhigh; /* Total high memory size */
    //                unsigned long freehigh;  /* Available high memory size */
    //                unsigned int mem_unit;   /* Memory unit size in bytes */
    //                char _f[20-2*sizeof(long)-sizeof(int)];
    //                                         /* Padding to 64 bytes */
    //            };
    //
    //        In the above structure, sizes of the memory and swap fields are given
    //        as multiples of mem_unit bytes.
    struct sysinfo info;
    memset(&info, 0, sizeof info);
    int err = sysinfo(&info);
    if (!err) {
        double f_load = 1.0 / (1 << SI_LOAD_SHIFT);
        if (verbose > 2) {
            qDebug() << "*** Sysinfo Stats ***";
            qDebug() << "nr of cpus      :" << get_nprocs();
            qDebug() << "uptime (h)      :" << info.uptime / 3600.;
            qDebug() << "load avg (1min) :" << info.loads[0] * f_load;
            qDebug() << "free RAM        :" << (1.0e-6 * info.freeram / info.mem_unit) << "Mb";
            qDebug() << "free swap       :" << (1.0e-6 * info.freeswap / info.mem_unit) << "Mb";
        }
        emit logParameter(LogParameter("systemNrCPUs", QString::number(get_nprocs()) + " ", LogParameter::LOG_ONCE));
        emit logParameter(LogParameter("systemUptime", QString::number(info.uptime / 3600.) + " h", LogParameter::LOG_LATEST));
        emit logParameter(LogParameter("systemFreeMem", QString::number(1e-6 * info.freeram / info.mem_unit) + " Mb", LogParameter::LOG_AVERAGE));
        emit logParameter(LogParameter("systemFreeSwap", QString::number(1e-6 * info.freeswap / info.mem_unit) + " Mb", LogParameter::LOG_AVERAGE));
        emit logParameter(LogParameter("systemLoadAvg", QString::number(info.loads[0] * f_load) + " ", LogParameter::LOG_AVERAGE));
    }
}

void Daemon::onUBXReceivedTimeTM2(const UbxTimeMarkStruct& tm)
{
    if (!tm.risingValid && !tm.fallingValid) {
        qDebug() << "Daemon::onUBXReceivedTimeTM2(const UbxTimeMarkStruct&): detected invalid time mark message; no rising or falling edge data";
        return;
    }
    static UbxTimeMarkStruct lastTimeMark {};

    long double dts = (tm.falling.tv_sec - tm.rising.tv_sec) * 1.0e9L;
    dts += (tm.falling.tv_nsec - tm.rising.tv_nsec);
    if ((dts > 0.0L) && tm.fallingValid) {
        histoMap["UbxEventLength"].fill(static_cast<double>(dts));
    }
    long double interval = (tm.rising.tv_sec - lastTimeMark.rising.tv_sec) * 1.0e9L;
    interval += (tm.rising.tv_nsec - lastTimeMark.rising.tv_nsec);
    if (interval < 1e12)
        histoMap["UbxEventInterval"].fill(static_cast<double>(1.0e-6L * interval));
    uint16_t diffCount = tm.evtCounter - lastTimeMark.evtCounter;
    emit timeMarkIntervalCountUpdate(diffCount, static_cast<double>(interval * 1.0e-9L));
    lastTimeMark = tm;

    // output is: rising falling timeAcc valid timeBase utcAvailable
    std::stringstream tempStream;
    tempStream << tm.rising << tm.falling << tm.accuracy_ns << " " << tm.evtCounter << " "
               << static_cast<short>(tm.valid) << " " << static_cast<short>(tm.timeBase) << " "
               << static_cast<short>(tm.utcAvailable);
    emit eventMessage(QString::fromStdString(tempStream.str()));

    if (!tm.risingValid || !tm.fallingValid) {
        qDebug() << "detected timemark message with reconstructed edge time (" << QString((tm.risingValid) ? "falling" : "rising") << ")";
        qDebug() << "msg:" << QString::fromStdString(tempStream.str());
    }

    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_UBX_TIMEMARK);
    (*tcpMessage.dStream) << tm;
    emit sendTcpMessage(tcpMessage);
}

void Daemon::updateOledDisplay()
{
    if (!oled_p || !oled_p->devicePresent())
        return;
    oled_p->clearDisplay();
    oled_p->setCursor(0, 2);
    oled_p->print("*Cosmic Shower Det.*\n");
    oled_p->printf("Rates %4.1f %4.1f /s\n", getRateFromCounts(AND_RATE), getRateFromCounts(XOR_RATE));
    if (temp_sensor_p && temp_sensor_p->probeDevicePresence()) {
        oled_p->printf("temp %4.2f %cC\n", temp_sensor_p->getTemperature(), DEGREE_CHARCODE);
    }
    oled_p->printf("%d(%d) Sats ", nrVisibleSats().toInt(), nrSats().toInt(), DEGREE_CHARCODE);
    oled_p->printf("%s\n", Gnss::FixType::name[fixStatus().toInt()]);
    oled_p->display();
}

void Daemon::onStatusLed1Event(int onTimeMs)
{
    emit message(std::make_shared<gpio_state>(GPIO_PINMAP[STATUS1], true));
    if (onTimeMs) {
        QTimer::singleShot(onTimeMs, [&]() {
            emit message(std::make_shared<gpio_state>(GPIO_PINMAP[STATUS1], false));
        });
    }
}

void Daemon::onStatusLed2Event(int onTimeMs)
{
    emit message(std::make_shared<gpio_state>(GPIO_PINMAP[STATUS2], true));
    if (onTimeMs) {
        QTimer::singleShot(onTimeMs, [&]() {
            emit message(std::make_shared<gpio_state>(GPIO_PINMAP[STATUS2], false));
        });
    }
}
