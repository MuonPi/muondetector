#include "mainwindow.h"

#include "calibform.h"
#include "calibscandialog.h"
#include "capnp/capnp_codec.h"
#include "gnssinfoform.h"
#include "gui/src/ui_mainwindow.h"
#include "histogramdataform.h"
#include "i2cform.h"
#include "logplotswidget.h"
#include "map.h"
#include "network/networkdiscovery.h"
#include "parametermonitorform.h"
#include "scanform.h"
#include "status.h"
#include "ubloxsettingsform.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QErrorMessage>
#include <QFile>
#include <QHostAddress>
#include <QKeyEvent>
#include <QMessageBox>
#include <QRegularExpression>
#include <QThread>
#include <boost/asio.hpp>
#include <data/commands/adc_mode_request_cmd.h>
#include <data/commands/adc_sample_request_cmd.h>
#include <data/commands/bias_dac_setting_cmd.h>
#include <data/commands/bias_switch_cmd.h>
#include <data/commands/burst_sampling_cmd.h>
#include <data/commands/calibration_cmd.h>
#include <data/commands/calibration_save_cmd.h>
#include <data/commands/dac_eeprom_set_cmd.h>
#include <data/commands/dac_request_cmd.h>
#include <data/commands/gain_switch_cmd.h>
#include <data/commands/gpio_rate_request_cmd.h>
#include <data/commands/gpio_rate_reset_cmd.h>
#include <data/commands/histogram_clear_cmd.h>
#include <data/commands/i2c_scan_bus_cmd.h>
#include <data/commands/i2c_stats_request_cmd.h>
#include <data/commands/mqtt_inhibit_cmd.h>
#include <data/commands/pca_switch_cmd.h>
#include <data/commands/polarity_switch_cmd.h>
#include <data/commands/preamp_switch_cmd.h>
#include <data/commands/temperature_request_cmd.h>
#include <data/commands/threshold_request_cmd.h>
#include <data/commands/threshold_setting_cmd.h>
#include <data/commands/ubx_default_config_cmd.h>
#include <data/commands/ubx_msg_poll_rate_cmd.h>
#include <data/commands/ubx_msg_rate_cmd.h>
#include <data/commands/ubx_reset_cmd.h>
#include <data/commands/ubx_save_cmd.h>
#include <data/events/adc_mode_event.h>
#include <data/events/adc_trace_event.h>
#include <data/events/ads1115_event.h>
#include <data/events/bias_switch_event.h>
#include <data/events/bias_voltage_event.h>
#include <data/events/calib_event.h>
#include <data/events/gain_switch_event.h>
#include <data/events/gpio_event.h>
#include <data/events/gpio_inhibit_event.h>
#include <data/events/gpio_rate_event.h>
#include <data/events/i2c_stats_event.h>
#include <data/events/lm75_event.h>
#include <data/events/mcp4728_event.h>
#include <data/events/mqtt_status_event.h>
#include <data/events/pca_switch_event.h>
#include <data/events/polarity_switch_event.h>
#include <data/events/preamp_switch_event.h>
#include <data/events/spi_stats_event.h>
#include <data/events/threshold_setting_event.h>
#include <data/events/ubx_event.h>
#include <data/events/version_event.h>
#include <gpio_pin_definitions.h>
#include <histogram.h>
#include <iostream>
#include <muondetector_structs.h>
#include <tcpmessage_keys.h>
#include <unordered_map>

using namespace std;

/// flash duration of and/xor label after hit
constexpr std::chrono::milliseconds gpioRateHoldInterval{50};
/// automatic rate poll interval
constexpr std::chrono::seconds gpioRatePollInterval{3};
/// TCP socket connection timeout
constexpr std::chrono::seconds CONNECTION_TIMEOUT{10};

QRegularExpression alphabetical("[a-z]+[A-Z]+");

QRegularExpression specialCharacters(
    // R"([\-`~!@#\$%\^&\*()_—\+=|:;<>«»\?/\{\}'"ß\\]+)"
    QString::fromUtf8("[\\-`~!@#\\$%\\^\\&\\*()_\\—\\+=|:;<>«»\\?/{}\'\"ß\\\\]+"));

namespace {
void sendPacketIfConnected(const std::shared_ptr<TcpConnection>& conn, TCP_MSG_KEY key,
                           std::vector<std::uint8_t> payload = {}) {
    if (!conn || !conn->isOpen()) {
        return;
    }
    conn->sendPacket(static_cast<std::uint16_t>(key), std::move(payload));
}

template <typename T>
void sendCmdIfConnected(const std::shared_ptr<TcpConnection>& conn, const T& cmd) {
    sendPacketIfConnected(conn, static_cast<TCP_MSG_KEY>(CapnpCodec<T>::messageKey()),
                          CapnpCodec<T>::encode(cmd));
}
} // namespace

MainWindow::MainWindow(std::shared_ptr<boost::asio::io_context> io, QWidget* parent)
    : QMainWindow(parent), decoderMap{buildDecoderMap()}, m_io{io}, ui(new Ui::MainWindow) {
    qRegisterMetaType<GnssPosStruct>("GnssPosStruct");
    qRegisterMetaType<bool>("bool");
    qRegisterMetaType<I2cDeviceEntry>("I2cDeviceEntry");
    qRegisterMetaType<CalibStruct>("CalibStruct");
    qRegisterMetaType<std::vector<GnssConfigStruct>>("std::vector<GnssConfigStruct>");
    qRegisterMetaType<std::vector<GnssSatellite>>("std::vector<GnssSatellite>");
    qRegisterMetaType<UbxTimePulseStruct>("UbxTimePulseStruct");
    qRegisterMetaType<GPIO_SIGNAL>("GPIO_SIGNAL");
    qRegisterMetaType<GnssMonHwStruct>("GnssMonHwStruct");
    qRegisterMetaType<GnssMonHw2Struct>("GnssMonHw2Struct");
    qRegisterMetaType<UbxTimeMarkStruct>("UbxTimeMarkStruct");
    qRegisterMetaType<int32_t>("int32_t");
    qRegisterMetaType<uint32_t>("uint32_t");
    qRegisterMetaType<uint16_t>("uint16_t");
    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<int8_t>("int8_t");
    qRegisterMetaType<std::chrono::duration<double>>("std::chrono::duration<double>");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<UbxDopStruct>("UbxDopStruct");
    qRegisterMetaType<timespec>("timespec");
    qRegisterMetaType<ADC_SAMPLING_MODE>("ADC_SAMPLING_MODE");
    qRegisterMetaType<PositionModeConfig>("PositionModeConfig");
    qRegisterMetaType<UbxMsgRates>("UbxMsgRates");

    ui->setupUi(this);
    this->setWindowTitle(
        QString("muondetector-gui  " + QString::fromStdString(MuonPi::Version::software.string())));

    QIcon icon(":/res/muon.ico");
    this->setWindowIcon(icon);
    setMaxThreshVoltage(1.0);

    // setup ipBox and load addresses etc.
    addresses = new QStandardItemModel(this);
    loadSettings(addresses);
    ui->ipBox->setModel(addresses);
    ui->ipBox->setCompleter(new QCompleter{});
    ui->ipBox->setEditable(true);

    // setup network discovery service
    auto networkDiscovery =
        new NetworkDiscovery(NetworkDiscovery::DeviceType::GUI, MuonPi::Settings::gui.port);
    connect(ui->networkSearchButton, &QPushButton::clicked, this, [this, networkDiscovery]() {
        const QList<QPair<quint16, QHostAddress>> devices;
        // TODO: Add result of network discovery items here
        if (addresses == nullptr) {
            return;
        }
        unsigned device_counter{};
        for (auto device : devices) {
            // check if device is not a GUI (might show other GUIs later on)
            if (device.first == static_cast<quint16>(NetworkDiscovery::DeviceType::GUI)) {
                continue;
            }
            device_counter++;
            // append to addresses if not already there
            if (addresses->findItems(device.second.toString()).isEmpty()) {
                auto row = new QStandardItem(device.second.toString());
                addresses->appendRow(row);
            }
        }
        connection_info("NetworkDiscovery (experimental): Found " +
                        QString::number(device_counter) + " devices");
    });

    // setup colors
    ui->ipStatusLabel->setStyleSheet("QLabel {color : darkGray;}");

    // setup event filter
    ui->ipBox->installEventFilter(this);
    ui->ipButton->installEventFilter(this);

    // setup signal/slots
    connect(ui->ipButton, &QPushButton::pressed, this, &MainWindow::onIpButtonClicked);

    // set timer for and/xor label color change after hit
    andTimer.setSingleShot(true);
    xorTimer.setSingleShot(true);
    andTimer.setInterval(gpioRateHoldInterval);
    xorTimer.setInterval(gpioRateHoldInterval);
    connect(&andTimer, &QTimer::timeout, this, &MainWindow::resetAndHit);
    connect(&xorTimer, &QTimer::timeout, this, &MainWindow::resetXorHit);

    ui->ANDHit->setFocusPolicy(Qt::NoFocus);
    ui->XORHit->setFocusPolicy(Qt::NoFocus);

    // set timer for automatic rate poll
    if (automaticRatePoll) {
        ratePollTimer.setInterval(gpioRatePollInterval);
        ratePollTimer.setSingleShot(false);
        connect(&ratePollTimer, &QTimer::timeout, this, &MainWindow::sendRequestGpioRates);
        connect(&ratePollTimer, &QTimer::timeout, this, &MainWindow::sendValueUpdateRequests);
        ratePollTimer.start();
    }

    connect(this, &MainWindow::daemonVersionReceived, this, &MainWindow::onDaemonVersionReceived);
    connect(this, &MainWindow::biasSwitchReceived, this, &MainWindow::onBiasSwitchReceived);

    // set all tabs
    ui->tabWidget->removeTab(0);
    Status* status = new Status(this);
    connect(this, &MainWindow::setUiEnabledStates, status, &Status::onUiEnabledStateChange);

    connect(this, &MainWindow::gpioRates, status, &Status::onGpioRatesReceived);
    connect(status, &Status::resetRateClicked, this,
            [this]() { sendCmdIfConnected(clientConn, GpioRateResetCmd{}); });
    connect(this, &MainWindow::adcSampleReceived, status, &Status::onAdcSampleReceived);
    connect(this, &MainWindow::dacReadbackReceived, status, &Status::onDacReadbackReceived);
    connect(status, &Status::inputSwitchChanged, this, &MainWindow::sendInputSwitch);
    connect(this, &MainWindow::inputSwitchReceived, status, &Status::onInputSwitchReceived);
    connect(this, &MainWindow::biasSwitchReceived, status, &Status::onBiasSwitchReceived);
    connect(status, &Status::biasSwitchChanged, this, &MainWindow::sendSetBiasStatus);
    connect(this, &MainWindow::preampSwitchReceived, status, &Status::onPreampSwitchReceived);
    connect(status, &Status::preamp1SwitchChanged, this, &MainWindow::sendPreamp1Switch);
    connect(status, &Status::preamp2SwitchChanged, this, &MainWindow::sendPreamp2Switch);
    connect(this, &MainWindow::gainSwitchReceived, status, &Status::onGainSwitchReceived);
    connect(status, &Status::gainSwitchChanged, this, &MainWindow::sendGainSwitch);
    connect(this, &MainWindow::temperatureReceived, status, &Status::onTemperatureReceived);
    connect(this, &MainWindow::triggerSelectionReceived, status,
            &Status::onTriggerSelectionReceived);
    connect(status, &Status::triggerSelectionChanged, this, &MainWindow::onTriggerSelectionChanged);
    connect(this, &MainWindow::timepulseReceived, status, &Status::onTimepulseReceived);
    connect(this, static_cast<void (MainWindow::*)(bool)>(&MainWindow::mqttStatusChanged), status,
            static_cast<void (Status::*)(bool)>(&Status::onMqttStatusChanged));
    connect(
        this,
        static_cast<void (MainWindow::*)(MuonPi::MqttHandler::Status)>(
            &MainWindow::mqttStatusChanged),
        status,
        static_cast<void (Status::*)(MuonPi::MqttHandler::Status)>(&Status::onMqttStatusChanged));

    ui->tabWidget->addTab(status, "Overview");

    UbloxSettingsForm* settings = new UbloxSettingsForm(this);
    connect(this, &MainWindow::setUiEnabledStates, settings,
            &UbloxSettingsForm::onUiEnabledStateChange);
    connect(this, &MainWindow::txBufReceived, settings, &UbloxSettingsForm::onTxBufReceived);
    connect(this, &MainWindow::txBufPeakReceived, settings,
            &UbloxSettingsForm::onTxBufPeakReceived);
    connect(this, &MainWindow::rxBufReceived, settings, &UbloxSettingsForm::onRxBufReceived);
    connect(this, &MainWindow::rxBufPeakReceived, settings,
            &UbloxSettingsForm::onRxBufPeakReceived);
    connect(this, &MainWindow::addUbxMsgRates, settings, &UbloxSettingsForm::addUbxMsgRates);
    connect(settings, &UbloxSettingsForm::sendRequestUbxMsgRates, this,
            &MainWindow::sendRequestUbxMsgRates);
    connect(settings, &UbloxSettingsForm::sendSetUbxMsgRateChanges, this,
            &MainWindow::sendSetUbxMsgRateChanges);
    connect(settings, &UbloxSettingsForm::sendUbxReset, this, &MainWindow::onSendUbxReset);
    connect(settings, &UbloxSettingsForm::sendUbxConfigDefault, this,
            [this]() { sendCmdIfConnected(clientConn, UbxConfigDefaultCmd{}); });
    connect(this, &MainWindow::gnssConfigsReceived, settings,
            &UbloxSettingsForm::onGnssConfigsReceived);
    connect(settings, &UbloxSettingsForm::setGnssConfigs, this, &MainWindow::onSetGnssConfigs);
    connect(this, &MainWindow::gpsTP5Received, settings, &UbloxSettingsForm::onTP5Received);
    connect(settings, &UbloxSettingsForm::setTP5Config, this, &MainWindow::onSetTP5Config);
    connect(settings, &UbloxSettingsForm::sendUbxSaveCfg, this,
            [this]() { sendCmdIfConnected(clientConn, UbxSaveCmd{}); });

    ui->tabWidget->addTab(settings, "Ublox Settings");

    Map* map = new Map(this);
    connect(this, &MainWindow::setUiEnabledStates, map, &Map::onUiEnabledStateChange);
    connect(this, &MainWindow::geodeticPos, map, &Map::onGeodeticPosReceived);
    connect(this, &MainWindow::positionModeConfigReceived, map, &Map::onPosConfigReceived);
    connect(map, &Map::posModeConfigChanged, this, &MainWindow::onPosModeConfigChanged);
    ui->tabWidget->addTab(map, "Map");

    I2cForm* i2cTab = new I2cForm(this);
    connect(this, &MainWindow::setUiEnabledStates, i2cTab, &I2cForm::onUiEnabledStateChange);
    connect(this, &MainWindow::i2cStatsReceived, i2cTab, &I2cForm::onI2cStatsReceived);
    connect(i2cTab, &I2cForm::i2cStatsRequest, this,
            [this]() { sendCmdIfConnected(clientConn, I2cStatsRequestCmd{}); });
    connect(i2cTab, &I2cForm::scanI2cBusRequest, this,
            [this]() { sendCmdIfConnected(clientConn, I2cScanBusCmd{}); });

    ui->tabWidget->addTab(i2cTab, "I2C bus");

    calib = new CalibForm(this);
    connect(this, &MainWindow::setUiEnabledStates, calib, &CalibForm::onUiEnabledStateChange);
    connect(this, &MainWindow::calibReceived, calib, &CalibForm::onCalibReceived);
    connect(calib, &CalibForm::calibRequest, this,
            [this]() { sendCmdIfConnected(clientConn, CalibRequestCmd{}); });
    connect(calib, &CalibForm::writeCalibToEeprom, this,
            [this]() { sendCmdIfConnected(clientConn, CalibSaveCmd{}); });
    connect(this, &MainWindow::adcSampleReceived, calib, &CalibForm::onAdcSampleReceived);
    connect(calib, &CalibForm::setBiasDacVoltage, this, &MainWindow::sendSetBiasVoltage);
    connect(calib, &CalibForm::setDacVoltage, this, &MainWindow::sendSetThresh);
    connect(calib, &CalibForm::updatedCalib, this, &MainWindow::onCalibUpdated);
    connect(calib, &CalibForm::setBiasSwitch, this, &MainWindow::sendSetBiasStatus);
    ui->tabWidget->addTab(calib, "Calibration");

    calibscandialog = new CalibScanDialog(this);
    calibscandialog->hide();
    connect(this, &MainWindow::calibReceived, calibscandialog, &CalibScanDialog::onCalibReceived);
    connect(this, &MainWindow::adcSampleReceived, calibscandialog,
            &CalibScanDialog::onAdcSampleReceived);

    GnssInfoForm* satsTab = new GnssInfoForm(this);
    connect(this, &MainWindow::setUiEnabledStates, satsTab, &GnssInfoForm::onUiEnabledStateChange);
    connect(this, &MainWindow::satsReceived, satsTab, &GnssInfoForm::onSatsReceived);
    connect(this, &MainWindow::timeAccReceived, satsTab, &GnssInfoForm::onTimeAccReceived);
    connect(this, &MainWindow::freqAccReceived, satsTab, &GnssInfoForm::onFreqAccReceived);
    connect(this, &MainWindow::intCounterReceived, satsTab, &GnssInfoForm::onIntCounterReceived);
    connect(this, &MainWindow::gpsMonHWReceived, satsTab, &GnssInfoForm::onGpsMonHWReceived);
    connect(this, &MainWindow::gpsMonHW2Received, satsTab, &GnssInfoForm::onGpsMonHW2Received);
    connect(this, &MainWindow::gpsVersionReceived, satsTab, &GnssInfoForm::onGpsVersionReceived);
    connect(this, &MainWindow::gpsFixReceived, satsTab, &GnssInfoForm::onGpsFixReceived);
    connect(this, &MainWindow::geodeticPos, satsTab, &GnssInfoForm::onGeodeticPosReceived);
    connect(this, &MainWindow::ubxUptimeReceived, satsTab, &GnssInfoForm::onUbxUptimeReceived);

    ui->tabWidget->addTab(satsTab, "GNSS Data");

    histogramDataForm* histoTab = new histogramDataForm(this);
    connect(this, &MainWindow::setUiEnabledStates, histoTab,
            &histogramDataForm::onUiEnabledStateChange);
    connect(this, &MainWindow::histogramReceived, histoTab,
            &histogramDataForm::onHistogramReceived);
    connect(histoTab, &histogramDataForm::histogramCleared, this, &MainWindow::onHistogramCleared);
    ui->tabWidget->addTab(histoTab, "Statistics");

    ParameterMonitorForm* paramTab = new ParameterMonitorForm(this);
    connect(this, &MainWindow::setUiEnabledStates, paramTab,
            &ParameterMonitorForm::onUiEnabledStateChange);
    connect(this, &MainWindow::adcSampleReceived, paramTab,
            &ParameterMonitorForm::onAdcSampleReceived);
    connect(this, &MainWindow::adcTraceReceived, paramTab,
            &ParameterMonitorForm::onAdcTraceReceived);
    connect(this, &MainWindow::dacReadbackReceived, paramTab,
            &ParameterMonitorForm::onDacReadbackReceived);
    connect(this, &MainWindow::inputSwitchReceived, paramTab,
            &ParameterMonitorForm::onInputSwitchReceived);
    connect(this, &MainWindow::biasSwitchReceived, paramTab,
            &ParameterMonitorForm::onBiasSwitchReceived);
    connect(this, &MainWindow::preampSwitchReceived, paramTab,
            &ParameterMonitorForm::onPreampSwitchReceived);
    connect(this, &MainWindow::polaritySwitchReceived, paramTab,
            &ParameterMonitorForm::onPolaritySwitchReceived);
    connect(this, &MainWindow::triggerSelectionReceived, paramTab,
            &ParameterMonitorForm::onTriggerSelectionReceived);
    connect(this, &MainWindow::temperatureReceived, paramTab,
            &ParameterMonitorForm::onTemperatureReceived);
    connect(this, &MainWindow::timeAccReceived, paramTab, &ParameterMonitorForm::onTimeAccReceived);
    connect(this, &MainWindow::freqAccReceived, paramTab, &ParameterMonitorForm::onFreqAccReceived);
    connect(this, &MainWindow::gainSwitchReceived, paramTab,
            &ParameterMonitorForm::onGainSwitchReceived);
    connect(this, &MainWindow::calibReceived, paramTab, &ParameterMonitorForm::onCalibReceived);
    connect(this, &MainWindow::timeMarkReceived, paramTab,
            &ParameterMonitorForm::onTimeMarkReceived);
    connect(this, &MainWindow::daemonVersionReceived, paramTab,
            &ParameterMonitorForm::onDaemonVersionReceived);
    connect(paramTab, &ParameterMonitorForm::adcModeChanged, this, &MainWindow::onAdcModeChanged);
    connect(paramTab, &ParameterMonitorForm::setDacVoltage, this, &MainWindow::sendSetThresh);
    connect(paramTab, &ParameterMonitorForm::preamp1EnableChanged, this,
            &MainWindow::sendPreamp1Switch);
    connect(paramTab, &ParameterMonitorForm::preamp2EnableChanged, this,
            &MainWindow::sendPreamp2Switch);
    connect(paramTab, &ParameterMonitorForm::biasEnableChanged, this,
            &MainWindow::sendSetBiasStatus);
    connect(paramTab, &ParameterMonitorForm::gainSwitchChanged, this, &MainWindow::sendGainSwitch);
    connect(paramTab, &ParameterMonitorForm::polarityChanged, this, &MainWindow::onPolarityChanged);
    connect(paramTab, &ParameterMonitorForm::timingSelectionChanged, this,
            &MainWindow::sendInputSwitch);
    connect(paramTab, &ParameterMonitorForm::triggerSelectionChanged, this,
            &MainWindow::onTriggerSelectionChanged);
    connect(paramTab, &ParameterMonitorForm::gpioInhibitChanged, this, &MainWindow::gpioInhibit);
    connect(paramTab, &ParameterMonitorForm::mqttInhibitChanged, this, &MainWindow::mqttInhibit);
    ui->tabWidget->addTab(paramTab, "Parameters");

    ScanForm* scanTab = new ScanForm(this);
    connect(this, &MainWindow::setUiEnabledStates, scanTab, &ScanForm::onUiEnabledStateChange);
    connect(this, &MainWindow::timeMarkReceived, scanTab, &ScanForm::onTimeMarkReceived);
    connect(this, &MainWindow::dacReadbackReceived, scanTab, &ScanForm::onDacReadbackReceived);
    connect(scanTab, &ScanForm::setThresholdVoltage, this, &MainWindow::sendSetThresh);
    connect(scanTab, &ScanForm::setBiasControlVoltage, this, &MainWindow::sendSetBiasVoltage);
    connect(scanTab, &ScanForm::gpioInhibitChanged, this, &MainWindow::gpioInhibit);
    connect(scanTab, &ScanForm::mqttInhibitChanged, this, &MainWindow::mqttInhibit);
    ui->tabWidget->addTab(scanTab, "Scans");

    LogPlotsWidget* logTab = new LogPlotsWidget(this);
    connect(this, &MainWindow::temperatureReceived, logTab, &LogPlotsWidget::onTemperatureReceived);
    connect(this, &MainWindow::timeAccReceived, logTab, &LogPlotsWidget::onTimeAccReceived);
    connect(this, &MainWindow::setUiEnabledStates, logTab, &LogPlotsWidget::onUiEnabledStateChange);
    connect(paramTab, &ParameterMonitorForm::biasVoltageCalculated, logTab,
            &LogPlotsWidget::onBiasVoltageCalculated);
    connect(paramTab, &ParameterMonitorForm::biasCurrentCalculated, logTab,
            &LogPlotsWidget::onBiasCurrentCalculated);

    connect(this, &MainWindow::gpioRates, logTab, &LogPlotsWidget::onGpioRatesReceived,
            Qt::QueuedConnection);

    connect(this, &MainWindow::logInfoReceived, logTab, &LogPlotsWidget::onLogInfoReceived);
    ui->tabWidget->addTab(logTab, "Log");

    const QStandardItemModel* model =
        dynamic_cast<QStandardItemModel*>(ui->biasControlTypeComboBox->model());
    QStandardItem* item = model->item(1);
    item->setEnabled(false);

    m_connection_timeout.setSingleShot(true);
    m_connection_timeout.setInterval(CONNECTION_TIMEOUT);
    connect(&m_connection_timeout, &QTimer::timeout, this,
            [this]() { this->connection_error(255, QString("connection timeout")); });
    // initialise all ui elements that will be inactive at start
    uiSetDisconnectedState();
}

MainWindow::~MainWindow() {
    emit closeConnection();
    saveSettings(addresses);
    delete ui;
}

void MainWindow::makeConnection(QString ipAddress, quint16 port) {
    tcp::socket clientSocket(*m_io);
    boost::system::error_code ec;
    auto server_ip = boost::asio::ip::make_address_v4(ipAddress.toStdString(), ec);
    if (ec) {
        QMessageBox::critical(this, "Connection Failed",
                              "Invalid IP: " + QString::fromStdString(ec.message()));
    }
    tcp::endpoint endpoint(server_ip, port);
    clientSocket.connect(endpoint, ec);
    if (ec) {
        QMessageBox::critical(this, "Connection Failed",
                              "client connect failed: " + QString::fromStdString(ec.message()));
        return;
    }

    clientConn = std::make_shared<TcpConnection>(std::move(clientSocket));
    auto weakConn = std::weak_ptr<TcpConnection>(clientConn);
    clientConn->setPacketHandler([weakConn, this](const TcpPacket& packet) {
        if (auto conn = weakConn.lock()) {
            decode(packet);
        }
    });
    clientConn->start();
}

void MainWindow::decode(const TcpPacket& packet) {
    auto it = decoderMap.find(static_cast<TCP_MSG_KEY>(packet.key));

    if (it != decoderMap.end()) {
        it->second(packet);
        updateUiProperties();
    } else {
        QMessageBox::critical(
            this, "Received Unknown Message",
            QString::fromStdString("Unknown message key: " + std::to_string(packet.key)));
    }
}

auto MainWindow::buildDecoderMap()
    -> std::unordered_map<TCP_MSG_KEY, std::function<void(const TcpPacket&)>> {
    return {// { // TODO: Make quit connection functioning properly
            //     TCP_MSG_KEY::MSG_QUIT_CONNECTION,
            //     [this](const TcpPacket& packet){
            //         auto event = CapnpCodec<>::decode(packet.payload);
            //         connectedToDemon = false;
            //     }
            // },
            {TCP_MSG_KEY::MSG_GPIO_EVENT,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<GpioEvent>::decode(packet.payload);
                 if (event.edge == EventEdge::Falling) {
                     return;
                 }

                 if (event.gpio_signal == EVT_AND) {
                     ui->ANDHit->setStyleSheet("QLabel {background-color: darkGreen;}");
                     andTimer.start();
                 } else if (event.gpio_signal == EVT_XOR) {
                     ui->XORHit->setStyleSheet("QLabel {background-color: darkGreen;}");
                     xorTimer.start();
                 } else if (event.gpio_signal == TIMEPULSE) {
                     emit timepulseReceived();
                 }
             }},
            {TCP_MSG_KEY::MSG_UBX_MSG_RATE,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<UbxMsgRates>::decode(packet.payload);
                 addUbxMsgRates(event);
             }},
            {TCP_MSG_KEY::MSG_THRESHOLD,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<ThresholdSettingEvent>::decode(packet.payload);
                 quint8 channel = event.channel;
                 float threshold = event.voltage;
                 if (threshold > maxThreshVoltage) {
                     sendSetThresh(channel, maxThreshVoltage);
                     return;
                 }
                 if (std::abs(sliderValues.at(channel) - (1e3 * threshold)) >
                     std::numeric_limits<float>::epsilon()) {
                     sliderValuesDirty = true;
                 }
                 sliderValues.at(channel) = 1e3 * threshold;
             }},
            {TCP_MSG_KEY::MSG_BIAS_VOLTAGE,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<BiasVoltageEvent>::decode(packet.payload);
                 biasDacVoltage = event.voltage;
                 // TODO: Update UI
             }},
            {TCP_MSG_KEY::MSG_BIAS_SWITCH,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<BiasSwitchEvent>::decode(packet.payload);
                 emit biasSwitchReceived(event.biasOn);
             }},
            {TCP_MSG_KEY::MSG_PREAMP_SWITCH,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<PreampSwitchEvent>::decode(packet.payload);
                 emit preampSwitchReceived(event.channel, event.state);
             }},
            {TCP_MSG_KEY::MSG_GAIN_SWITCH,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<GainSwitchEvent>::decode(packet.payload);
                 emit gainSwitchReceived(event.state);
             }},
            {TCP_MSG_KEY::MSG_PCA_SWITCH,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<PcaSwitchEvent>::decode(packet.payload);
                 if (event.pcaPortMask != static_cast<int>(TIMING_MUX_SELECTION::UNDEFINED)) {
                     emit inputSwitchReceived(static_cast<TIMING_MUX_SELECTION>(event.pcaPortMask));
                 }
             }},
            {TCP_MSG_KEY::MSG_POLARITY_SWITCH,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<PolaritySwitchEvent>::decode(packet.payload);
                 emit polaritySwitchReceived(event.pol1, event.pol2);
             }},
            {TCP_MSG_KEY::MSG_EVENTTRIGGER,
             [this](const TcpPacket& packet) {
                 // TODO: Understand what happens here logically
                 // auto event = CapnpCodec<EventTriggerEvent>::decode(packet.payload);
                 // emit triggerSelectionReceived(event.signal);
             }},
            {TCP_MSG_KEY::MSG_GPIO_RATE,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<GpioRateEvent>::decode(packet.payload);
                 QVector<QPointF> out;
                 out.reserve(static_cast<int>(event.rate.size()));

                 for (const auto& [x, y] : event.rate) {
                     out.emplace_back(x, y);
                 }
                 emit gpioRates(event.whichRate, std::move(out));
             }},
            {TCP_MSG_KEY::MSG_GEO_POS,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<GnssPosStruct>::decode(packet.payload);
                 emit geodeticPos(event);
             }},
            {TCP_MSG_KEY::MSG_ADC_SAMPLE,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<Ads1115Event>::decode(packet.payload);
                 emit adcSampleReceived(event.channel, event.voltage);
             }},
            {TCP_MSG_KEY::MSG_ADC_TRACE,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<AdcTraceEvent>::decode(packet.payload);
                 emit adcTraceReceived(std::move(event.adcSampleBuffer));
             }},
            {TCP_MSG_KEY::MSG_DAC_READBACK,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<MCP4728Event>::decode(packet.payload);
                 //  emit dacReadbackReceived(std::move(event.data));
             }},
            {TCP_MSG_KEY::MSG_TEMPERATURE,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<LM75Event>::decode(packet.payload);
                 emit temperatureReceived(event.temperature);
             }},
            {TCP_MSG_KEY::MSG_I2C_STATS,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<I2CStatsEvent>::decode(packet.payload);
                 emit i2cStatsReceived(event.bytesRead, event.bytesWritten, event.deviceList);
             }},
            {TCP_MSG_KEY::MSG_SPI_STATS,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<SPIStatsEvent>::decode(packet.payload);
                 emit spiStatsReceived(event.spiPresent);
             }},
            {TCP_MSG_KEY::MSG_CALIB_SET,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<CalibEvent>::decode(packet.payload);
                 emit calibReceived(event.valid, event.eepromValid, event.id, event.calibList);
             }},
            {TCP_MSG_KEY::MSG_GNSS_SATS,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<NavSat>::decode(packet.payload);
                 emit satsReceived(event.satellites);
             }},
            {TCP_MSG_KEY::MSG_UBX_GNSS_CONFIG,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<CfgGNSS>::decode(packet.payload);
                 emit gnssConfigsReceived(event.numTrkChHw, event.configs);
             }},
            // { // TODO: Build processor for time accuracy, frequency accuracy and uptime
            //     TCP_MSG_KEY::MSG_UBX_TIME_ACCURACY,
            //     [this](const TcpPacket& packet){
            //         auto event = CapnpCodec<>::decode(packet.payload);
            //         emit timeAccReceived(event.tAcc);
            //     }
            // },
            // {
            //     TCP_MSG_KEY::MSG_UBX_FREQ_ACCURACY,
            //     [this](const TcpPacket& packet){
            //         auto event = CapnpCodec<>::decode(packet.payload);
            //         emit freqAccReceived(event.fAcc);
            //     }
            // },
            // {
            //     TCP_MSG_KEY::MSG_UBX_UPTIME,
            //     [this](const TcpPacket& packet){
            //         auto event = CapnpCodec<>::decode(packet.payload);
            //         emit ubxUptimeReceived(val);
            //     }
            // },
            // { // Deprecated since sent through MSG_UBX_TIMEMARK
            //     TCP_MSG_KEY::MSG_UBX_EVENTCOUNTER,
            //     [this](const TcpPacket& packet){
            //         auto event = CapnpCodec<>::decode(packet.payload);
            //         emit intCounterReceived(event.cnt);
            //         ui->eventCounter->setText(QString::number(event.cnt));
            //     }
            // },
            {TCP_MSG_KEY::MSG_UBX_TIMEMARK,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<UbxTimeMarkStruct>::decode(packet.payload);
                 emit intCounterReceived(event.evtCounter);
                 emit timeMarkReceived(event);
             }},
            // { // Currently not used
            //     TCP_MSG_KEY::MSG_UBX_TXBUF,
            //     [this](const TcpPacket& packet){
            //         auto event = CapnpCodec<MonTx>::decode(packet.payload);
            //         emit txBufPeakReceived(event.tPeakUsage);
            //     }
            // },
            // {
            //     TCP_MSG_KEY::MSG_UBX_RXBUF,
            //     [this](const TcpPacket& packet){
            //         auto event = CapnpCodec<MonRx>::decode(packet.payload);
            //         emit txBufPeakReceived(event.tUsage);
            //     }
            // },
            {TCP_MSG_KEY::MSG_UBX_TXBUF_PEAK,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<MonTx>::decode(packet.payload);
                 emit txBufPeakReceived(event.tUsage);
             }},
            {TCP_MSG_KEY::MSG_UBX_RXBUF_PEAK,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<MonRx>::decode(packet.payload);
                 emit txBufPeakReceived(event.tPeakUsage);
             }},
            {TCP_MSG_KEY::MSG_UBX_MONHW,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<GnssMonHwStruct>::decode(packet.payload);
                 emit gpsMonHWReceived(event);
             }},
            {TCP_MSG_KEY::MSG_UBX_MONHW2,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<GnssMonHw2Struct>::decode(packet.payload);
                 emit gpsMonHW2Received(event);
             }},
            {TCP_MSG_KEY::MSG_UBX_VERSION,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<GpsVersion>::decode(packet.payload);
                 emit gpsVersionReceived(QString::fromStdString(event.swString),
                                         QString::fromStdString(event.hwString),
                                         QString::fromStdString(event.prot));
             }},
            {TCP_MSG_KEY::MSG_UBX_FIXSTATUS,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<NavStatus>::decode(packet.payload);
                 emit gpsFixReceived(event.gpsFix);
             }},
            {TCP_MSG_KEY::MSG_UBX_CFG_TP5,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<UbxTimePulseStruct>::decode(packet.payload);
                 emit gpsTP5Received(event);
             }},
            {TCP_MSG_KEY::MSG_HISTOGRAM,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<Histogram>::decode(packet.payload);
                 emit histogramReceived(event);
             }},
            {TCP_MSG_KEY::MSG_ADC_MODE,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<AdcModeEvent>::decode(packet.payload);
                 emit adcModeReceived(event.mode);
             }},
            {TCP_MSG_KEY::MSG_LOG_INFO,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<LogInfoStruct>::decode(packet.payload);
                 emit logInfoReceived(event);
             }},
            {TCP_MSG_KEY::MSG_MQTT_STATUS,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<MqttStatusEvent>::decode(packet.payload);
                 emit mqttStatusChanged(event.connected);
             }},
            {TCP_MSG_KEY::MSG_GPIO_INHIBIT,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<GpioInhibitEvent>::decode(packet.payload);
                 emit gpioInhibitReceived(event.inhibit);
             }},
            {TCP_MSG_KEY::MSG_VERSION,
             [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<VersionEvent>::decode(packet.payload);
                 emit daemonVersionReceived(event.hw_ver, event.sw_ver);
             }},
            {TCP_MSG_KEY::MSG_POSITION_MODEL, [this](const TcpPacket& packet) {
                 auto event = CapnpCodec<PositionModeConfig>::decode(packet.payload);
                 emit positionModeConfigReceived(event);
             }}};
}

void MainWindow::onTriggerSelectionChanged(GPIO_SIGNAL signal) {
    // TcpMessage tcpMessage(TCP_MSG_KEY::MSG_EVENTTRIGGER);
    // *(tcpMessage.dStream) << signal;
    // emit sendTcpMessage(tcpMessage);
    // sendRequest(TCP_MSG_KEY::MSG_EVENTTRIGGER_REQUEST);
}

bool MainWindow::saveSettings(QStandardItemModel* model) {
    QString file_location{QStandardPaths::writableLocation(QStandardPaths::CacheLocation)};
    if (!QDir(file_location).exists()) {
        if (!QDir().mkpath(file_location)) {
            qWarning() << "Could not create cache path";
            return false;
        }
    }
    QFile file{file_location + "/muondetector-gui.save"};
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "file open failed in 'WriteOnly' mode at location " << file.fileName();
        return false;
    }

    QDataStream stream{&file};
    qint32 n{model->rowCount()};
    stream << n;
    for (int i = 0; i < n; i++) {
        model->item(i)->write(stream);
    }
    file.close();
    return true;
}

bool MainWindow::loadSettings(QStandardItemModel* model) {
    QFile file{QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
               "/muondetector-gui.save"};
    if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
        qDebug() << "file open failed in 'ReadOnly' mode at location " << file.fileName();
        return false;
    }
    QDataStream stream(&file);
    qint32 n;
    stream >> n;
    for (int i = 0; i < n; i++) {
        model->appendRow(new QStandardItem());
        model->item(i)->read(stream);
    }
    file.close();
    return true;
}

bool MainWindow::eventFilter(QObject* object, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        auto combobox = dynamic_cast<QComboBox*>(object);
        if (combobox == ui->ipBox) {
            if (ke->key() == Qt::Key_Delete) {
                ui->ipBox->removeItem(ui->ipBox->currentIndex());
            } else if (ke->key() == Qt::Key_Enter || ke->key() == Qt::Key_Return) {
                onIpButtonClicked();
            } else {
                return QObject::eventFilter(object, event);
            }
        } else {
            return QObject::eventFilter(object, event);
        }
        return true;
    } else {
        return QObject::eventFilter(object, event);
    }
}

void MainWindow::sendRequestUbxMsgRates() {
    sendCmdIfConnected(clientConn, UbxMsgPollRateCmd{static_cast<UBX_MSG::msg_id>(0)});
}

void MainWindow::sendSetBiasVoltage(float voltage) {
    BiasVoltageEvent cmd{};
    cmd.voltage = voltage;
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_BIAS_VOLTAGE,
                          CapnpCodec<BiasVoltageEvent>::encode(cmd));
}

void MainWindow::sendSetBiasStatus(bool status) {
    BiasSwitchEvent cmd{};
    cmd.biasOn = status;
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_BIAS_SWITCH,
                          CapnpCodec<BiasSwitchEvent>::encode(cmd));
}

void MainWindow::sendGainSwitch(bool status) {
    GainSwitchEvent cmd{};
    cmd.state = status;
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_GAIN_SWITCH,
                          CapnpCodec<GainSwitchEvent>::encode(cmd));
}

void MainWindow::sendPreamp1Switch(bool status) {
    PreampSwitchEvent cmd{};
    cmd.channel = 0;
    cmd.state = status;
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_PREAMP_SWITCH,
                          CapnpCodec<PreampSwitchEvent>::encode(cmd));
}

void MainWindow::sendPreamp2Switch(bool status) {
    PreampSwitchEvent cmd{};
    cmd.channel = 1;
    cmd.state = status;
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_PREAMP_SWITCH,
                          CapnpCodec<PreampSwitchEvent>::encode(cmd));
}

void MainWindow::sendSetThresh(uint8_t channel, float value) {
    ThresholdSettingCmd cmd{};
    cmd.channel = channel;
    cmd.threshold = value;
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_THRESHOLD,
                          CapnpCodec<ThresholdSettingCmd>::encode(cmd));
}

void MainWindow::sendSetUbxMsgRateChanges(QMap<uint16_t, int> changes) {
    for (auto it = changes.begin(); it != changes.end(); ++it) {
        UbxMsgRateCmd cmd{};
        cmd.id = static_cast<UBX_MSG::msg_id>(it.key());
        cmd.port = 1;
        cmd.rate = static_cast<std::uint8_t>(std::clamp(it.value(), 0, 255));
        sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_UBX_MSG_RATE,
                              CapnpCodec<UbxMsgRateCmd>::encode(cmd));
    }
}

void MainWindow::onSendUbxReset() {
    sendCmdIfConnected(clientConn, UbxResetCmd{});
}

void MainWindow::onHistogramCleared(QString histogramName) {
    sendCmdIfConnected(clientConn, HistogramClearCmd{histogramName.toStdString()});
}

void MainWindow::onAdcModeChanged(ADC_SAMPLING_MODE mode) {
    AdcModeEvent cmd{};
    cmd.mode = static_cast<std::uint8_t>(mode);
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_ADC_MODE,
                          CapnpCodec<AdcModeEvent>::encode(cmd));
}

void MainWindow::onRateScanStart(uint8_t ch) {
    Q_UNUSED(ch)
    sendCmdIfConnected(clientConn, StartBurstSampling{10, 100});
}

void MainWindow::onSetGnssConfigs(const std::vector<GnssConfigStruct>& configList) {
    CfgGNSS cmd{};
    cmd.numTrkChHw = static_cast<std::uint8_t>(configList.size());
    cmd.numConfigBlocks = static_cast<std::uint8_t>(configList.size());
    cmd.configs = configList;
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_UBX_GNSS_CONFIG,
                          CapnpCodec<CfgGNSS>::encode(cmd));
}

void MainWindow::onSetTP5Config(const UbxTimePulseStruct& tp) {
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_UBX_CFG_TP5,
                          CapnpCodec<UbxTimePulseStruct>::encode(tp));
}

void MainWindow::sendRequestGpioRates() {
    sendCmdIfConnected(clientConn, GpioRateRequestCmd{0});
    sendCmdIfConnected(clientConn, GpioRateRequestCmd{1});
}

void MainWindow::sendRequestGpioRateBuffer() {
    sendCmdIfConnected(clientConn, GpioRateRequestCmd{0});
    sendCmdIfConnected(clientConn, GpioRateRequestCmd{1});
}

void MainWindow::resetAndHit() {
    ui->ANDHit->setStyleSheet("QLabel {background-color: Window;}");
}
void MainWindow::resetXorHit() {
    ui->XORHit->setStyleSheet("QLabel {background-color: Window;}");
}

void MainWindow::uiSetDisconnectedState() {
    sliderValuesDirty = true;
    // set button and color of label
    ui->ipStatusLabel->setStyleSheet("QLabel {color: darkGray;}");
    ui->ipStatusLabel->setText("not connected");
    ui->ipButton->setText("connect");
    ui->ipButton->setEnabled(true);
    ui->ipBox->setEnabled(true);
    // disable all relevant objects of mainwindow
    ui->discr1Slider->setValue(0);
    ui->discr1Edit->clear();
    ui->discr2Slider->setValue(0);
    ui->discr2Edit->clear();
    ui->biasPowerLabel->setStyleSheet("QLabel {color: darkGray;}");
    ui->tabWidget->setEnabled(false);
    ui->controlWidget->setEnabled(false);
    // disable other widgets
    emit setUiEnabledStates(false);
    m_connection_timeout.stop();
}

void MainWindow::uiSetConnectedState() {
    sliderValuesDirty = true;
    // change color and text of labels and buttons
    ui->tabWidget->setEnabled(true);
    ui->controlWidget->setEnabled(true);
    ui->ipStatusLabel->setStyleSheet("QLabel {color: darkGreen;}");
    ui->ipStatusLabel->setText("connected");
    ui->ipButton->setText("disconnect");
    ui->ipButton->setEnabled(true);
    ui->ipBox->setDisabled(true);
    // enable other widgets
    emit setUiEnabledStates(true);
}

void MainWindow::updateUiProperties() {
    mouseHold = true;

    if (sliderValuesDirty) {
        ui->discr1Slider->setEnabled(true);
        ui->discr1Slider->setValue(sliderValues.at(0));
        ui->discr1Edit->setEnabled(true);

        ui->discr2Slider->setEnabled(true);
        ui->discr2Slider->setValue(sliderValues.at(1));
        ui->discr2Edit->setEnabled(true);

        sliderValuesDirty = false;
    }
    double biasVoltage = biasCalibOffset + biasDacVoltage * biasCalibSlope;
    ui->biasVoltageSlider->blockSignals(true);
    ui->biasVoltageDoubleSpinBox->blockSignals(true);
    ui->biasVoltageDoubleSpinBox->setValue(biasVoltage);
    ui->biasVoltageSlider->setValue(100 * biasVoltage / maxBiasVoltage);
    ui->biasVoltageSlider->blockSignals(false);
    ui->biasVoltageDoubleSpinBox->blockSignals(false);
    // equation:
    // UBias = c1*UDac + c0
    // (UBias - c0)/c1 = UDac

    ui->ANDHit->setEnabled(true);
    ui->XORHit->setEnabled(true);
    ui->rate1->setEnabled(true);
    ui->rate2->setEnabled(true);
    ui->biasPowerButton->setEnabled(true);
    ui->biasPowerLabel->setEnabled(true);
    mouseHold = false;
}

void MainWindow::onBiasSwitchReceived(bool biasEnabled) {
    if (biasEnabled) {
        ui->biasPowerButton->setText("Disable");
        ui->biasPowerLabel->setText("Bias ON");
        ui->biasPowerLabel->setStyleSheet("QLabel {background-color: darkGreen; color: white;}");
    } else {
        ui->biasPowerButton->setText("Enable");
        ui->biasPowerLabel->setText("Bias OFF");
        ui->biasPowerLabel->setStyleSheet("QLabel {background-color: red; color: white;}");
    }
}

void MainWindow::connected() {
    connectedToDemon = true;
    saveSettings(addresses);
    uiSetConnectedState();
    sendValueUpdateRequests();
    sendCmdIfConnected(clientConn, PreampSwitchRequestCmd{0});
    sendCmdIfConnected(clientConn, PreampSwitchRequestCmd{1});
    sendCmdIfConnected(clientConn, GainSwitchRequestCmd{});
    sendCmdIfConnected(clientConn, ThresholdRequestCmd{});
    sendCmdIfConnected(clientConn, PcaSwitchRequestCmd{});
    sendRequestUbxMsgRates();
    sendRequestGpioRateBuffer();
    sendCmdIfConnected(clientConn, CalibRequestCmd{});
    sendCmdIfConnected(clientConn, AdcModeRequestCmd{});
    sendCmdIfConnected(clientConn, PolaritySwitchRequestCmd{});
}

void MainWindow::connection_info(const QString message) {
    ui->ipStatusLabel->setStyleSheet("");
    ui->ipStatusLabel->setText(message);
}

void MainWindow::connection_error(int error_code, const QString message) {
    uiSetDisconnectedState();
    ui->ipStatusLabel->setStyleSheet("QLabel {color: red;}");
    ui->ipStatusLabel->setText("Connection error: (" + QString::number(error_code) + ") '" +
                               message + "'");
}

void MainWindow::sendValueUpdateRequests() {
    sendCmdIfConnected(clientConn, BiasVoltageRequestCmd{});
    sendCmdIfConnected(clientConn, BiasSwitchRequestCmd{});
    for (int i = 0; i < 4; i++)
        sendCmdIfConnected(clientConn, DacRequestCmd{static_cast<std::uint8_t>(i)});
    for (int i = 1; i < 4; i++)
        sendCmdIfConnected(clientConn, AdcSampleRequestCmd{static_cast<std::uint8_t>(i)});
    sendCmdIfConnected(clientConn, TemperatureRequestCmd{});
    sendCmdIfConnected(clientConn, I2cStatsRequestCmd{});
}

void MainWindow::onIpButtonClicked() {
    if (connectedToDemon) {
        // it is connected and the button shows "disconnect" -> here comes disconnect code
        connectedToDemon = false;
        emit closeConnection();
        andTimer.stop();
        xorTimer.stop();
        uiSetDisconnectedState();
        return;
    }
    QString ipBoxText = ui->ipBox->currentText();
    QStringList ipAndPort = ipBoxText.split(':');
    if (ipAndPort.size() > 2 || ipAndPort.size() < 1) {
        QString errorMess = "error, size of ipAndPort not 1 or 2";
        errorM.showMessage(errorMess);
        return;
    }
    QString ipAddress = ipAndPort.at(0);
    if (ipAddress == "local" || ipAddress == "localhost") {
        ipAddress = "127.0.0.1";
    }
    int port{MuonPi::Settings::gui.port};
    if (ipAndPort.size() == 2) {
        port = ipAndPort.at(1).toInt();
    }
    makeConnection(ipAddress, port);
    if (!ui->ipBox->currentText().isEmpty() &&
        ui->ipBox->findText(ui->ipBox->currentText()) == -1) {
        // if text not already in there, put it in there
        ui->ipBox->addItem(ui->ipBox->currentText());
    }
}

void MainWindow::on_discr1Save_clicked() {
    sendSetThresh(0, 1e-3 * ui->discr1Edit->value());
    sliderValuesDirty = true;
}

void MainWindow::on_discr2Save_clicked() {
    sendSetThresh(1, 1e-3 * ui->discr2Edit->value());
    sliderValuesDirty = true;
}

void MainWindow::setMaxThreshVoltage(float voltage) {
    // we have 0.5 mV resolution so we have (int)(mVolts)*2 steps on the slider
    // the '+0.5' is to round up or down like in mathematics
    maxThreshVoltage = voltage;

    ui->discr1Slider->setUpperBound(1e3 * voltage);
    ui->discr2Slider->setUpperBound(1e3 * voltage);
    ui->discr1Edit->setMaximum(1e3 * voltage);
    ui->discr2Edit->setMaximum(1e3 * voltage);

    if (sliderValues.at(0) > voltage) {
        sendSetThresh(0, voltage);
    }
    if (sliderValues.at(1) > voltage) {
        sendSetThresh(1, voltage);
    }
}

float MainWindow::parseValue(QString text) {
    // ignores everything that is not a number or at least most of it
    text = text.simplified();
    text = text.replace(" ", "");
    text = text.remove(alphabetical);
    text = text.replace(",", ".");
    text = text.remove(specialCharacters);
    bool ok;
    float value = text.toFloat(&ok);
    if (!ok) {
        errorM.showMessage("failed to parse discr1Edit to float");
        return -1;
    }
    return value;
}

void MainWindow::on_saveDacButton_clicked() {
    sendCmdIfConnected(clientConn, DacEepromSetCmd{});
}

void MainWindow::on_biasPowerButton_clicked() {
    sendSetBiasStatus(!biasON);
}

void MainWindow::sendInputSwitch(TIMING_MUX_SELECTION sel) {
    if (sel == TIMING_MUX_SELECTION::UNDEFINED) {
        return;
    }
    PcaSwitchEvent cmd{};
    cmd.pcaPortMask = static_cast<std::uint8_t>(sel);
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_PCA_SWITCH,
                          CapnpCodec<PcaSwitchEvent>::encode(cmd));
}

void MainWindow::on_biasVoltageSlider_sliderReleased() {
    mouseHold = false;
    on_biasVoltageSlider_valueChanged(ui->biasVoltageSlider->value());
}

void MainWindow::on_biasVoltageSlider_valueChanged(int value) {
    if (!mouseHold) {
        double biasVoltage = (double) value / ui->biasVoltageSlider->maximum() * maxBiasVoltage;
        if (fabs(biasCalibSlope) < 1e-5)
            return;
        double dacVoltage = (biasVoltage - biasCalibOffset) / biasCalibSlope;
        if (dacVoltage < 0.)
            dacVoltage = 0.;
        if (dacVoltage > 3.3)
            dacVoltage = 3.3;
        sendSetBiasVoltage(dacVoltage);
    }
    // equation:
    // UBias = c1*UDac + c0
    // (UBias - c0)/c1 = UDac
}

void MainWindow::on_biasVoltageSlider_sliderPressed() {
    mouseHold = true;
}

void MainWindow::onCalibUpdated(const std::vector<CalibStruct>& items) {
    if (calib == nullptr)
        return;

    if (!items.empty()) {
        CalibEvent event{};
        event.calibList = items;
        sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_CALIB_SET,
                              CapnpCodec<CalibEvent>::encode(event));
    }

    uint8_t flags = calib->getCalibParameter("CALIB_FLAGS").toUInt();
    bool calibedBias = false;
    if (flags & CalibStruct::CALIBFLAGS_VOLTAGE_COEFFS)
        calibedBias = true;

    const QStandardItemModel* model =
        dynamic_cast<QStandardItemModel*>(ui->biasControlTypeComboBox->model());
    QStandardItem* item = model->item(1);

    item->setEnabled(calibedBias);
    ui->biasControlTypeComboBox->setCurrentIndex((calibedBias) ? 1 : 0);
}

void MainWindow::on_biasControlTypeComboBox_currentIndexChanged(int index) {
    if (index == 1) {
        if (calib == nullptr)
            return;
        QString str = calib->getCalibParameter("COEFF0");
        if (!str.size())
            return;
        double c0 = str.toDouble();
        str = calib->getCalibParameter("COEFF1");
        if (!str.size())
            return;
        double c1 = str.toDouble();
        biasCalibOffset = c0;
        biasCalibSlope = c1;
        minBiasVoltage = 0.;
        maxBiasVoltage = 40.;
        ui->biasVoltageDoubleSpinBox->setMaximum(maxBiasVoltage);
        ui->biasVoltageDoubleSpinBox->setSingleStep(0.1);
    } else {
        biasCalibOffset = 0.;
        biasCalibSlope = 1.;
        minBiasVoltage = 0.;
        maxBiasVoltage = 3.3;
        ui->biasVoltageDoubleSpinBox->setMaximum(maxBiasVoltage);
        ui->biasVoltageDoubleSpinBox->setSingleStep(0.01);
    }
    sendCmdIfConnected(clientConn, BiasVoltageRequestCmd{});
}

void MainWindow::on_biasVoltageDoubleSpinBox_valueChanged(double arg1) {
    double biasVoltage = arg1;
    if (fabs(biasCalibSlope) < 1e-5)
        return;
    double dacVoltage = (biasVoltage - biasCalibOffset) / biasCalibSlope;
    if (dacVoltage < 0.)
        dacVoltage = 0.;
    if (dacVoltage > 3.3)
        dacVoltage = 3.3;
    sendSetBiasVoltage(dacVoltage);
}

void MainWindow::gpioInhibit(bool inhibit) {
    GpioInhibitEvent cmd{};
    cmd.inhibit = inhibit;
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_GPIO_INHIBIT,
                          CapnpCodec<GpioInhibitEvent>::encode(cmd));
}

void MainWindow::mqttInhibit(bool inhibit) {
    MqttInhibitCmd cmd{};
    cmd.inhibit = inhibit;
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_MQTT_INHIBIT,
                          CapnpCodec<MqttInhibitCmd>::encode(cmd));
}

void MainWindow::onPolarityChanged(bool pol1, bool pol2) {
    PolaritySwitchEvent cmd{pol1, pol2};
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_POLARITY_SWITCH,
                          CapnpCodec<PolaritySwitchEvent>::encode(cmd));
}

void MainWindow::onPosModeConfigChanged(const PositionModeConfig& posconfig) {
    sendPacketIfConnected(clientConn, TCP_MSG_KEY::MSG_POSITION_MODEL,
                          CapnpCodec<PositionModeConfig>::encode(posconfig));
}

void MainWindow::onDaemonVersionReceived(MuonPi::Version::Version /*hw_ver*/,
                                         MuonPi::Version::Version /*sw_ver*/) {
}
