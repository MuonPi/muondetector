#include <mainwindow.h>
#include <ui_mainwindow.h>
#include <QThread>
#include <QFile>
#include <QKeyEvent>
#include <QDebug>
#include <QErrorMessage>
#include <calib_struct.h>
#include <gnsssatellite.h>
#include <ublox_structs.h>
#include <settings.h>
#include <status.h>
#include <tcpmessage_keys.h>
#include <map.h>
#include <i2cform.h>
#include <calibform.h>
#include <gpssatsform.h>
#include <iostream>
#include <histogram.h>
#include "histogramdataform.h"

using namespace std;

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

QDataStream& operator << (QDataStream& out, const CalibStruct& calib)
{
    out << QString::fromStdString(calib.name) << QString::fromStdString(calib.type)
     << (quint16)calib.address << QString::fromStdString(calib.value);
    return out;
}

QDataStream& operator >> (QDataStream& in, GnssSatellite& sat)
{
/*
	int fGnssId=0, fSatId=0, fCnr=0, fElev=0, fAzim=0;
	float fPrRes=0.;
	int fQuality=0, fHealth=0;
	int fOrbitSource=0;
	bool fUsed=false, fDiffCorr=false, fSmoothed=false;
*/
	in >> sat.fGnssId >> sat.fSatId >> sat.fCnr >> sat.fElev >> sat.fAzim
		>> sat.fPrRes >> sat.fQuality >> sat.fHealth >> sat.fOrbitSource
		>> sat.fUsed >> sat.fDiffCorr >> sat.fSmoothed;
	return in;
}

QDataStream& operator >> (QDataStream& in, UbxTimePulseStruct& tp)
{
    in >> tp.tpIndex >> tp.version >> tp.antCableDelay >> tp.rfGroupDelay
	>> tp.freqPeriod >> tp.freqPeriodLock >> tp.pulseLenRatio >> tp.pulseLenRatioLock
	>> tp.userConfigDelay >> tp.flags;
    return in;
}

QDataStream& operator << (QDataStream& out, const UbxTimePulseStruct& tp)
{
    out << tp.tpIndex << tp.version << tp.antCableDelay << tp.rfGroupDelay
	<< tp.freqPeriod << tp.freqPeriodLock << tp.pulseLenRatio << tp.pulseLenRatioLock
	<< tp.userConfigDelay << tp.flags;
    return out;
}

QDataStream& operator >> (QDataStream& in, Histogram& h)
{
    h.clear();
    QString name,unit;
	in >> name >> h.fMin >> h.fMax >> h.fUnderflow >> h.fOverflow >> h.fNrBins;
	h.setName(name.toStdString());
	for (int i=0; i<h.fNrBins; i++) {
		in >> h.fHistogramMap[i];
	}
    in >> unit;
    h.setUnit(unit.toStdString());
    return in;
}

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
    qRegisterMetaType<TcpMessage>("TcpMessage");
    qRegisterMetaType<GeodeticPos>("GeodeticPos");
    qRegisterMetaType<bool>("bool");
    qRegisterMetaType<I2cDeviceEntry>("I2cDeviceEntry");
    qRegisterMetaType<CalibStruct>("CalibStruct");
    qRegisterMetaType<std::vector<GnssSatellite>>("std::vector<GnssSatellite>");
    qRegisterMetaType<UbxTimePulseStruct>("UbxTimePulseStruct");
    qRegisterMetaType<GPIO_PIN>("GPIO_PIN");

    ui->setupUi(this);
	ui->discr1Layout->setAlignment(ui->discr1Slider, Qt::AlignHCenter);
    ui->discr2Layout->setAlignment(ui->discr2Slider, Qt::AlignHCenter); // aligns the slider in their vertical layout centered
#if defined(Q_OS_UNIX)
        QIcon icon("/usr/share/pixmaps/muon.ico");
#elif defined(Q_OS_WIN)
        QIcon icon("muon.ico");
#else
        QIcon icon("muon.ico");
#endif
	this->setWindowIcon(icon);
    setMaxThreshVoltage(1.0);

    // setup ipBox and load addresses etc.
    addresses = new QStandardItemModel(this);
    loadSettings(addresses);
	ui->ipBox->setModel(addresses);
	ui->ipBox->setAutoCompletion(true);
	ui->ipBox->setEditable(true);
	//ui->ipBox->installEventFilter(this);

	// setup colors
	ui->ipStatusLabel->setStyleSheet("QLabel {color : darkGray;}");/*
	ui->discr1Hit->setStyleSheet("QLabel {background-color : darkRed;}");
	ui->discr2Hit->setStyleSheet("QLabel {background-color : darkRed;}");*/

	// setup event filter
	ui->ipBox->installEventFilter(this);
	ui->ipButton->installEventFilter(this);

	// set timer for and/xor label color change after hit
    int timerInterval = 100; // in msec
	andTimer.setSingleShot(true);
	xorTimer.setSingleShot(true);
	andTimer.setInterval(timerInterval);
	xorTimer.setInterval(timerInterval);
	connect(&andTimer, &QTimer::timeout, this, &MainWindow::resetAndHit);
	connect(&xorTimer, &QTimer::timeout, this, &MainWindow::resetXorHit);

    // set timer for automatic rate poll
    if (automaticRatePoll){
        ratePollTimer.setInterval(5000);
        ratePollTimer.setSingleShot(false);
        connect(&ratePollTimer, &QTimer::timeout, this, &MainWindow::sendRequestGpioRates);
        connect(&ratePollTimer, &QTimer::timeout, this, &MainWindow::sendValueUpdateRequests);
        ratePollTimer.start();
    }

    // set all tabs
    ui->tabWidget->removeTab(0);
    Status *status = new Status(this);
    connect(this, &MainWindow::setUiEnabledStates, status, &Status::onUiEnabledStateChange);
    connect(this, &MainWindow::gpioRates, status, &Status::onGpioRatesReceived);
    connect(status, &Status::resetRateClicked, this, [this](){ this->sendRequest(resetRateSig); } );
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
    connect(this, &MainWindow::triggerSelectionReceived, status, &Status::onTriggerSelectionReceived);
    connect(status, &Status::triggerSelectionChanged, this, &MainWindow::onTriggerSelectionChanged);
    connect(this, &MainWindow::timepulseReceived, status, &Status::onTimepulseReceived);

    ui->tabWidget->addTab(status,"Overview");

    Settings *settings = new Settings(this);
    connect(this, &MainWindow::setUiEnabledStates, settings, &Settings::onUiEnabledStateChange);
    connect(this, &MainWindow::txBufReceived, settings, &Settings::onTxBufReceived);
    connect(this, &MainWindow::txBufPeakReceived, settings, &Settings::onTxBufPeakReceived);
    connect(this, &MainWindow::rxBufReceived, settings, &Settings::onRxBufReceived);
    connect(this, &MainWindow::rxBufPeakReceived, settings, &Settings::onRxBufPeakReceived);
    connect(this, &MainWindow::addUbxMsgRates, settings, &Settings::addUbxMsgRates);
    connect(settings, &Settings::sendRequestUbxMsgRates, this, &MainWindow::sendRequestUbxMsgRates);
    connect(settings, &Settings::sendSetUbxMsgRateChanges, this, &MainWindow::sendSetUbxMsgRateChanges);
    connect(settings, &Settings::sendUbxReset, this, &MainWindow::onSendUbxReset);
    connect(settings, &Settings::sendUbxConfigDefault, this, [this](){ this->sendRequest(ubxConfigureDefaultSig); } );
    connect(this, &MainWindow::gnssConfigsReceived, settings, &Settings::onGnssConfigsReceived);
    connect(settings, &Settings::setGnssConfigs, this, &MainWindow::onSetGnssConfigs);
    connect(this, &MainWindow::gpsTP5Received, settings, &Settings::onTP5Received);
    connect(settings, &Settings::setTP5Config, this, &MainWindow::onSetTP5Config);
    connect(settings, &Settings::sendUbxSaveCfg, this, [this](){ this->sendRequest(ubxSaveCfgSig); } );

    ui->tabWidget->addTab(settings,"Ublox Settings");

    Map *map = new Map(this);
    ui->tabWidget->addTab(map, "Map");
    connect(this, &MainWindow::geodeticPos, map, &Map::onGeodeticPosReceived);



    I2cForm *i2cTab = new I2cForm(this);
    connect(this, &MainWindow::setUiEnabledStates, i2cTab, &I2cForm::onUiEnabledStateChange);
    connect(this, &MainWindow::i2cStatsReceived, i2cTab, &I2cForm::onI2cStatsReceived);
    connect(i2cTab, &I2cForm::i2cStatsRequest, this, [this]() { this->sendRequest(i2cStatsRequestSig); } );
    connect(i2cTab, &I2cForm::scanI2cBusRequest, this, [this]() { this->sendRequest(i2cScanBusRequestSig); } );

    ui->tabWidget->addTab(i2cTab,"I2C bus");

    calib = new CalibForm(this);
    connect(this, &MainWindow::setUiEnabledStates, calib, &CalibForm::onUiEnabledStateChange);
    connect(this, &MainWindow::calibReceived, calib, &CalibForm::onCalibReceived);
    connect(calib, &CalibForm::calibRequest, this, [this]() { this->sendRequest(calibRequestSig); } );
    connect(calib, &CalibForm::writeCalibToEeprom, this, [this]() { this->sendRequest(calibWriteEepromSig); } );
    connect(this, &MainWindow::adcSampleReceived, calib, &CalibForm::onAdcSampleReceived);
    connect(calib, &CalibForm::setBiasDacVoltage, this, &MainWindow::sendSetBiasVoltage);
    connect(calib, &CalibForm::updatedCalib, this, &MainWindow::onCalibUpdated);
    ui->tabWidget->addTab(calib,"Calibration");


    GpsSatsForm *satsTab = new GpsSatsForm(this);
    connect(this, &MainWindow::setUiEnabledStates, satsTab, &GpsSatsForm::onUiEnabledStateChange);
    connect(this, &MainWindow::satsReceived, satsTab, &GpsSatsForm::onSatsReceived);
    connect(this, &MainWindow::timeAccReceived, satsTab, &GpsSatsForm::onTimeAccReceived);
    connect(this, &MainWindow::freqAccReceived, satsTab, &GpsSatsForm::onFreqAccReceived);
    connect(this, &MainWindow::intCounterReceived, satsTab, &GpsSatsForm::onIntCounterReceived);
    connect(this, &MainWindow::gpsMonHWReceived, satsTab, &GpsSatsForm::onGpsMonHWReceived);
    connect(this, &MainWindow::gpsMonHW2Received, satsTab, &GpsSatsForm::onGpsMonHW2Received);
    connect(this, &MainWindow::gpsVersionReceived, satsTab, &GpsSatsForm::onGpsVersionReceived);
    connect(this, &MainWindow::gpsFixReceived, satsTab, &GpsSatsForm::onGpsFixReceived);
    connect(this, &MainWindow::geodeticPos, satsTab, &GpsSatsForm::onGeodeticPosReceived);
    connect(this, &MainWindow::ubxUptimeReceived, satsTab, &GpsSatsForm::onUbxUptimeReceived);


/*
//    connect(this, &MainWindow::setUiEnabledStates, settings, &Settings::onUiEnabledStateChange);
    connect(this, &MainWindow::calibReceived, calibTab, &CalibForm::onCalibReceived);
    connect(calibTab, &CalibForm::calibRequest, this, [this]() { this->sendRequest(calibRequestSig); } );
    connect(calibTab, &CalibForm::writeCalibToEeprom, this, [this]() { this->sendRequest(calibWriteEepromSig); } );
    connect(this, &MainWindow::adcSampleReceived, calibTab, &CalibForm::onAdcSampleReceived);
*/    
    ui->tabWidget->addTab(satsTab,"GNSS Data");


    histogramDataForm *histoTab = new histogramDataForm(this);
    connect(this, &MainWindow::setUiEnabledStates, histoTab, &histogramDataForm::onUiEnabledStateChange);
    connect(this, &MainWindow::histogramReceived, histoTab, &histogramDataForm::onHistogramReceived);
    ui->tabWidget->addTab(histoTab,"Statistics");


    //sendRequest(calibRequestSig);

    //settings->show();
	// set menu bar actions
    //connect(ui->actionsettings, &QAction::triggered, this, &MainWindow::settings_clicked);

    const QStandardItemModel *model = dynamic_cast<QStandardItemModel*>(ui->biasControlTypeComboBox->model());
    QStandardItem *item = model->item(1);
    item->setEnabled(false);
    // initialise all ui elements that will be inactive at start
    uiSetDisconnectedState();
}

MainWindow::~MainWindow()
{
	emit closeConnection();
    saveSettings(addresses);
    delete ui;
}

void MainWindow::makeConnection(QString ipAddress, quint16 port) {
    // add popup windows for errors!!!
	QThread *tcpThread = new QThread();
	if (!tcpConnection) {
		delete(tcpConnection);
	}
	tcpConnection = new TcpConnection(ipAddress, port, verbose);
	tcpConnection->moveToThread(tcpThread);
	connect(tcpThread, &QThread::started, tcpConnection, &TcpConnection::makeConnection);
    connect(tcpThread, &QThread::finished, tcpThread, &TcpConnection::deleteLater);
	connect(tcpConnection, &TcpConnection::connected, this, &MainWindow::connected);
    connect(this, &MainWindow::closeConnection, tcpConnection, &TcpConnection::closeThisConnection);
    connect(this, &MainWindow::sendTcpMessage, tcpConnection, &TcpConnection::sendTcpMessage);
    connect(tcpConnection, &TcpConnection::receivedTcpMessage, this, &MainWindow::receivedTcpMessage);
    tcpThread->start();
}

void MainWindow::onTriggerSelectionChanged(GPIO_PIN signal)
{
    TcpMessage tcpMessage(eventTriggerSig);
    *(tcpMessage.dStream) << signal;
    emit sendTcpMessage(tcpMessage);
    sendRequest(eventTriggerRequestSig);
}

bool MainWindow::saveSettings(QStandardItemModel *model) {
#if defined(Q_OS_UNIX)
    QFile file(QString("/usr/share/muondetector-gui/ipAddresses.save"));
#elif defined(Q_OS_WIN)
    QFile file(QString("ipAddresses.save"));
#else
    QFile file(QString("ipAddresses.save"));
#endif
    if (!file.open(QIODevice::ReadWrite)) {
        qDebug() << "file open failed in 'ReadWrite' mode at location " << file.fileName();
		return false;
	}
	QDataStream stream(&file);
	qint32 n(model->rowCount());
	stream << n;
	for (int i = 0; i < n; i++) {
		model->item(i)->write(stream);
	}
	file.close();
	return true;
}

bool MainWindow::loadSettings(QStandardItemModel* model) {
#if defined(Q_OS_UNIX)
    QFile file(QString("/usr/share/muondetector-gui/ipAddresses.save"));
#elif defined(Q_OS_WIN)
    QFile file(QString("ipAddresses.save"));
#else
    QFile file(QString("ipAddresses.save"));
#endif
	if (!file.open(QIODevice::ReadOnly)) {
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

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *ke = static_cast<QKeyEvent *>(event);
		auto combobox = dynamic_cast<QComboBox *>(object);
		if (combobox == ui->ipBox) {
			if (ke->key() == Qt::Key_Delete) {
				ui->ipBox->removeItem(ui->ipBox->currentIndex());
			}
		}
		if (ke->key() == Qt::Key_Escape) {
			QCoreApplication::quit();
			//this->deleteLater();
		}
		if (ke->key() == Qt::Key_Enter) {
			this->on_ipButton_clicked();
		}
		return false;
	}
	else {
		return false;
	}
}

void MainWindow::receivedTcpMessage(TcpMessage tcpMessage) {
    quint16 msgID = tcpMessage.getMsgID();
	if (msgID == gpioPinSig) {
        unsigned int gpioPin;
        *(tcpMessage.dStream) >> gpioPin;
        receivedGpioRisingEdge((GPIO_PIN)gpioPin);
        return;
	}
	if (msgID == ubxMsgRate) {
		QMap<uint16_t, int> msgRateCfgs;
        *(tcpMessage.dStream) >> msgRateCfgs;
		emit addUbxMsgRates(msgRateCfgs);
		return;
    }
    if (msgID == threshSig){
        quint8 channel;
        float threshold;
        *(tcpMessage.dStream) >> channel >> threshold;
        if (threshold > maxThreshVoltage){
            sendSetThresh(channel,maxThreshVoltage);
            return;
        }
        sliderValues[channel] = (int)(2000 * threshold);
        updateUiProperties();
        return;
    }
    if (msgID == biasVoltageSig){
        *(tcpMessage.dStream) >> biasDacVoltage;
        updateUiProperties();
        return;
    }
    if (msgID == biasSig){
        *(tcpMessage.dStream) >> biasON;
        emit biasSwitchReceived(biasON);
        updateUiProperties();
        return;
    }
    if (msgID == preampSig){
        quint8 channel;
        bool state;
        *(tcpMessage.dStream) >> channel >> state;
        emit preampSwitchReceived(channel, state);
        updateUiProperties();
        return;
    }
    if (msgID == gainSwitchSig){
        bool gainSwitch;
        *(tcpMessage.dStream) >> gainSwitch;
        emit gainSwitchReceived(gainSwitch);
        updateUiProperties();
        return;
    }
    if (msgID == pcaChannelSig){
        *(tcpMessage.dStream) >> pcaPortMask;
        //status->setInputSwitchButtonGroup(pcaPortMask);
        emit inputSwitchReceived(pcaPortMask);
        updateUiProperties();
        return;
    }
    if (msgID == eventTriggerSig){
        unsigned int signal;
        *(tcpMessage.dStream) >> signal;
        emit triggerSelectionReceived((GPIO_PIN)signal);
        //updateUiProperties();
        return;
    }
    if (msgID == gpioRateSig){
        quint8 whichRate;
        QVector<QPointF> rate;
        *(tcpMessage.dStream) >> whichRate >> rate;
        float rateYValue;
        if (!rate.empty()){
            rateYValue = rate.at(rate.size()-1).y();
        }else{
            rateYValue = 0.0;
        }
        if (whichRate == 0){
            ui->rate1->setText(QString::number(rateYValue,'g',3)+"/s");
        }
        if (whichRate == 1){
            ui->rate2->setText(QString::number(rateYValue,'g',3)+"/s");
        }
        emit gpioRates(whichRate, rate);
        updateUiProperties();
        return;
    }
    if (msgID == quitConnectionSig){
        connectedToDemon = false;
        uiSetDisconnectedState();
    }
    if (msgID == geodeticPosSig){
        GeodeticPos pos;
        *(tcpMessage.dStream) >> pos.iTOW >> pos.lon >> pos.lat
                >> pos.height >> pos.hMSL >> pos.hAcc >> pos.vAcc;
        emit geodeticPos(pos);
    }
    if (msgID == adcSampleSig){
        quint8 channel;
        float value;
        *(tcpMessage.dStream) >> channel >> value;
        emit adcSampleReceived(channel, value);
        updateUiProperties();
        return;
    }
    if (msgID == dacReadbackSig){
        quint8 channel;
        float value;
        *(tcpMessage.dStream) >> channel >> value;
        emit dacReadbackReceived(channel, value);
        updateUiProperties();
        return;
    }
    if (msgID == temperatureSig){
        float value;
        *(tcpMessage.dStream) >> value;
        emit temperatureReceived(value);
        updateUiProperties();
        return;
    }
    if (msgID == i2cStatsSig){
		quint8 nrDevices=0;
		quint32 bytesRead = 0;
		quint32 bytesWritten = 0;
    	*(tcpMessage.dStream) >> nrDevices >> bytesRead >> bytesWritten;

		QVector<I2cDeviceEntry> deviceList;
		for (uint8_t i=0; i<nrDevices; i++)
		{
			uint8_t addr = 0;
			QString title = "none";
			uint8_t status = 0;
			*(tcpMessage.dStream) >> addr >> title >> status;
			I2cDeviceEntry entry;
			entry.address=addr;
			entry.name = title;
			entry.status=status;
			deviceList.push_back(entry);
		}
        emit i2cStatsReceived(bytesRead, bytesWritten, deviceList);
        //updateUiProperties();
        return;
    }
    if (msgID == calibSetSig){
		quint16 nrPars=0;
		quint64 id = 0;
		bool valid = false;
		bool eepromValid = 0;
    	*(tcpMessage.dStream) >> valid >> eepromValid >> id >> nrPars;

		QVector<CalibStruct> calibList;
		for (uint8_t i=0; i<nrPars; i++)
		{
			CalibStruct item;
			*(tcpMessage.dStream) >> item;
			calibList.push_back(item);
		}
        emit calibReceived(valid, eepromValid, id, calibList);
        return;
    }
    if (msgID == gpsSatsSig){
		int nrSats=0;    	
		*(tcpMessage.dStream) >> nrSats;

		QVector<GnssSatellite> satList;
		for (uint8_t i=0; i<nrSats; i++)
		{
			GnssSatellite sat;
			*(tcpMessage.dStream) >> sat;
			satList.push_back(sat);
		}
        emit satsReceived(satList);
        return;
    }
    if (msgID == gnssConfigSig){
        int numTrkCh=0;
        int nrConfigs=0;

        *(tcpMessage.dStream) >> numTrkCh >> nrConfigs;

        QVector<GnssConfigStruct> configList;
        for (int i=0; i<nrConfigs; i++)
        {
            GnssConfigStruct config;
            *(tcpMessage.dStream) >> config.gnssId >> config.resTrkCh >>
                config.maxTrkCh >> config.flags;
            configList.push_back(config);
        }
        emit gnssConfigsReceived(numTrkCh, configList);
        return;
    }
    if (msgID == gpsTimeAccSig){
        quint32 acc=0;
        *(tcpMessage.dStream) >> acc;
        emit timeAccReceived(acc);
        return;
    }
    if (msgID == gpsFreqAccSig){
        quint32 acc=0;
        *(tcpMessage.dStream) >> acc;
        emit freqAccReceived(acc);
        return;
    }
    if (msgID == gpsIntCounterSig){
        quint32 cnt=0;
        *(tcpMessage.dStream) >> cnt;
        emit intCounterReceived(cnt);
        return;
    }
    if (msgID == gpsUptimeSig){
        quint32 val=0;
        *(tcpMessage.dStream) >> val;
        emit ubxUptimeReceived(val);
        return;
    }
    if (msgID == gpsTxBufSig){
        quint8 val=0;
        *(tcpMessage.dStream) >> val;
        emit txBufReceived(val);
        if (!tcpMessage.dStream->atEnd()) {
            *(tcpMessage.dStream) >> val;
            emit txBufPeakReceived(val);
        }
        return;
    }
    if (msgID == gpsRxBufSig){
        quint8 val=0;
        *(tcpMessage.dStream) >> val;
        emit rxBufReceived(val);
        if (!tcpMessage.dStream->atEnd()) {
            *(tcpMessage.dStream) >> val;
            emit rxBufPeakReceived(val);
        }
        return;
    }
    if (msgID == gpsTxBufPeakSig){
        quint8 val=0;
        *(tcpMessage.dStream) >> val;
        emit txBufPeakReceived(val);
        return;
    }
    if (msgID == gpsRxBufPeakSig){
        quint8 val=0;
        *(tcpMessage.dStream) >> val;
        emit rxBufPeakReceived(val);
        return;
    }
    if (msgID == gpsMonHWSig){
        quint16 noise=0;
        quint16 agc=0;
        quint8 antStatus=0;
        quint8 antPower=0;
        quint8 jamInd=0;
        quint8 flags=0;
        *(tcpMessage.dStream) >> noise >> agc >> antStatus >> antPower >> jamInd >> flags;
        emit gpsMonHWReceived(noise,agc,antStatus,antPower,jamInd,flags);
        return;
    }
    if (msgID == gpsMonHW2Sig){
        qint8 ofsI=0, ofsQ=0;
        quint8 magI=0, magQ=0;
        quint8 cfgSrc=0;
        *(tcpMessage.dStream) >> ofsI >> magI >> ofsQ >> magQ >> cfgSrc;
        //qDebug()<<"ofsI="<<ofsI<<" magI="<<magI<<"ofsQ="<<ofsQ<<" magQ="<<magQ<<" cfgSrc="<<cfgSrc;
        emit gpsMonHW2Received(ofsI, magI, ofsQ, magQ, cfgSrc);
        return;
    }
    if (msgID == gpsVersionSig){
        QString sw="";
        QString hw="";
        QString pv="";
        *(tcpMessage.dStream) >> sw >> hw >> pv;
        emit gpsVersionReceived(sw, hw, pv);
        return;
    }
    if (msgID == gpsFixSig){
        quint8 val=0;
        *(tcpMessage.dStream) >> val;
        emit gpsFixReceived(val);
        return;
    }
    if (msgID == gpsCfgTP5Sig){
        UbxTimePulseStruct tp;
        *(tcpMessage.dStream) >> tp;
        emit gpsTP5Received(tp);
        return;
    }
    if (msgID == histogramSig){
        Histogram h;
        *(tcpMessage.dStream) >> h;
        emit histogramReceived(h);
        return;
    }
}

void MainWindow::sendRequest(quint16 requestSig){
    TcpMessage tcpMessage(requestSig);
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::sendRequest(quint16 requestSig, quint8 par){
    TcpMessage tcpMessage(requestSig);
    *(tcpMessage.dStream) << par;
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::sendRequestUbxMsgRates(){
    TcpMessage tcpMessage(ubxMsgRateRequest);
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::sendSetBiasVoltage(float voltage){
    TcpMessage tcpMessage(biasVoltageSig);
    *(tcpMessage.dStream) << voltage;
    emit sendTcpMessage(tcpMessage);
    emit sendRequest(dacRequestSig, 2);
//    emit sendRequest(adcSampleRequestSig, 2);
//    emit sendRequest(adcSampleRequestSig, 3);
}

void MainWindow::sendSetBiasStatus(bool status){
    TcpMessage tcpMessage(biasSig);
    *(tcpMessage.dStream) << status;
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::sendGainSwitch(bool status){
    TcpMessage tcpMessage(gainSwitchSig);
    *(tcpMessage.dStream) << status;
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::sendPreamp1Switch(bool status){
    TcpMessage tcpMessage(preampSig);
    *(tcpMessage.dStream) << (quint8)0 << status;
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::sendPreamp2Switch(bool status){
    TcpMessage tcpMessage(preampSig);
    *(tcpMessage.dStream) << (quint8)1 << status;
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::sendSetThresh(uint8_t channel, float value){
    TcpMessage tcpMessage(threshSig);
    *(tcpMessage.dStream) << channel << value;
    emit sendTcpMessage(tcpMessage);
    emit sendRequest(dacRequestSig, channel);
}

void MainWindow::sendSetUbxMsgRateChanges(QMap<uint16_t, int> changes){
    TcpMessage tcpMessage(ubxMsgRate);
    *(tcpMessage.dStream) << changes;
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::onSendUbxReset()
{
    TcpMessage tcpMessage(ubxResetSig);
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::onSetGnssConfigs(const QVector<GnssConfigStruct>& configList){
    TcpMessage tcpMessage(gnssConfigSig);
    int N=configList.size();
    *(tcpMessage.dStream) << (int)N;
    for (int i=0; i<N; i++){
        *(tcpMessage.dStream) << configList[i].gnssId<<configList[i].resTrkCh
                              << configList[i].maxTrkCh<<configList[i].flags;
    }
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::onSetTP5Config(const UbxTimePulseStruct &tp)
{
    TcpMessage tcpMessage(gpsCfgTP5Sig);
    *(tcpMessage.dStream) << tp;
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::sendRequestGpioRates(){
    TcpMessage xorRateRequest(gpioRateRequestSig);
    *(xorRateRequest.dStream) << (quint16)5 << (quint8)0;
    emit sendTcpMessage(xorRateRequest);
    TcpMessage andRateRequest(gpioRateRequestSig);
    *(andRateRequest.dStream) << (quint16)5 << (quint8)1;
    emit sendTcpMessage(andRateRequest);
}

void MainWindow::sendRequestGpioRateBuffer(){
    TcpMessage xorRateRequest(gpioRateRequestSig);
    *(xorRateRequest.dStream) << (quint16)0 << (quint8)0;
    emit sendTcpMessage(xorRateRequest);
    TcpMessage andRateRequest(gpioRateRequestSig);
    *(andRateRequest.dStream) << (quint16)0 << (quint8)1;
    emit sendTcpMessage(andRateRequest);
}

void MainWindow::receivedGpioRisingEdge(GPIO_PIN pin) {
	if (pin == EVT_AND) {
        ui->ANDHit->setStyleSheet("QLabel {background-color: darkGreen;}");
		andTimer.start();
    } else if (pin == EVT_XOR) {
        ui->XORHit->setStyleSheet("QLabel {background-color: darkGreen;}");
		xorTimer.start();
    } else if (pin == TIMEPULSE) {
        emit timepulseReceived();
    }
}

void MainWindow::resetAndHit() {
    ui->ANDHit->setStyleSheet("QLabel {background-color: Window;}");
}
void MainWindow::resetXorHit() {
    ui->XORHit->setStyleSheet("QLabel {background-color: Window;}");
}

void MainWindow::uiSetDisconnectedState() {
	// set button and color of label
	ui->ipStatusLabel->setStyleSheet("QLabel {color: darkGray;}");
	ui->ipStatusLabel->setText("not connected");
	ui->ipButton->setText("connect");
	ui->ipBox->setEnabled(true);
    // disable all relevant objects of mainwindow
	ui->discr1Label->setStyleSheet("QLabel {color: darkGray;}");
	ui->discr2Label->setStyleSheet("QLabel {color: darkGray;}");
	ui->discr1Slider->setValue(0);
	ui->discr1Slider->setDisabled(true);
	ui->discr1Edit->clear();
	ui->discr1Edit->setDisabled(true);
	ui->discr2Slider->setValue(0);
	ui->discr2Slider->setDisabled(true);
	ui->discr2Edit->clear();
	ui->discr2Edit->setDisabled(true);
	ui->ANDHit->setDisabled(true);
    ui->ANDHit->setStyleSheet("QLabel {background-color: Window;}");
	ui->XORHit->setDisabled(true);
    ui->XORHit->setStyleSheet("QLabel {background-color: Window;}");
    ui->rate1->setDisabled(true);
    ui->rate2->setDisabled(true);
	ui->biasPowerLabel->setDisabled(true);
	ui->biasPowerLabel->setStyleSheet("QLabel {color: darkGray;}");
	ui->biasPowerButton->setDisabled(true);
    // disable other widgets
    emit setUiEnabledStates(false);
}

void MainWindow::uiSetConnectedState() {
	// change color and text of labels and buttons
	ui->ipStatusLabel->setStyleSheet("QLabel {color: darkGreen;}");
	ui->ipStatusLabel->setText("connected");
	ui->ipButton->setText("disconnect");
	ui->ipBox->setDisabled(true);
	ui->discr1Label->setStyleSheet("QLabel {color: black;}");
	ui->discr2Label->setStyleSheet("QLabel {color: black;}");
    // enable other widgets
    emit setUiEnabledStates(true);
}

void MainWindow::updateUiProperties() {
    mouseHold = true;

    ui->discr1Slider->setEnabled(true);
    ui->discr1Slider->setValue(sliderValues.at(0));
    ui->discr1Edit->setEnabled(true);
    ui->discr1Edit->setText(QString::number(sliderValues.at(0) / 2.0) + "mV");

    ui->discr2Slider->setEnabled(true);
    ui->discr2Slider->setValue(sliderValues.at(1));
    ui->discr2Edit->setEnabled(true);
    ui->discr2Edit->setText(QString::number(sliderValues.at(1) / 2.0) + "mV");
    double biasVoltage = biasCalibOffset + biasDacVoltage*biasCalibSlope;
    ui->biasVoltageSlider->blockSignals(true);
    ui->biasVoltageDoubleSpinBox->blockSignals(true);
    ui->biasVoltageDoubleSpinBox->setValue(biasVoltage);
    ui->biasVoltageSlider->setValue(100*biasVoltage/maxBiasVoltage);
    ui->biasVoltageSlider->blockSignals(false);
    ui->biasVoltageDoubleSpinBox->blockSignals(false);
    // equation:
    // UBias = c1*UDac + c0
    // (UBias - c0)/c1 = UDac

	ui->ANDHit->setEnabled(true);
    //ui->ANDHit->setStyleSheet("QLabel {background-color: darkRed; color: white;}");
	ui->XORHit->setEnabled(true);
    //ui->XORHit->setStyleSheet("QLabel {background-color: darkRed; color: white;}");
    ui->rate1->setEnabled(true);
    ui->rate2->setEnabled(true);
	ui->biasPowerButton->setEnabled(true);
	ui->biasPowerLabel->setEnabled(true);
    if (biasON) {
		ui->biasPowerButton->setText("Disable Bias");
		ui->biasPowerLabel->setText("Bias ON");
		ui->biasPowerLabel->setStyleSheet("QLabel {background-color: darkGreen; color: white;}");
	}
	else {
		ui->biasPowerButton->setText("Enable Bias");
		ui->biasPowerLabel->setText("Bias OFF");
		ui->biasPowerLabel->setStyleSheet("QLabel {background-color: red; color: white;}");
	}
	mouseHold = false;
}

void MainWindow::connected() {
    connectedToDemon = true;
    saveSettings(addresses);
    uiSetConnectedState();
    sendRequest(biasVoltageRequestSig);
    sendRequest(biasRequestSig);
    sendRequest(preampRequestSig,0);
    sendRequest(preampRequestSig,1);
    sendRequest(gainSwitchRequestSig);
    sendRequest(threshRequestSig);
    sendRequest(dacRequestSig,0);
    sendRequest(dacRequestSig,1);
    sendRequest(dacRequestSig,2);
    sendRequest(dacRequestSig,3);
    //sendRequest(adcSampleRequestSig,0);
    sendRequest(adcSampleRequestSig,1);
    sendRequest(adcSampleRequestSig,2);
    sendRequest(adcSampleRequestSig,3);
    sendRequest(pcaChannelRequestSig);
    sendRequestUbxMsgRates();
    sendRequestGpioRateBuffer();
    sendRequest(temperatureRequestSig);
    sendRequest(i2cStatsRequestSig);
    sendRequest(calibRequestSig);
}


void MainWindow::sendValueUpdateRequests() {
    sendRequest(biasVoltageRequestSig);
    sendRequest(biasRequestSig);
//    sendRequest(preampRequestSig,0);
//    sendRequest(preampRequestSig,1);
//    sendRequest(threshRequestSig);
    sendRequest(dacRequestSig,0);
    sendRequest(dacRequestSig,1);
    sendRequest(dacRequestSig,2);
    sendRequest(dacRequestSig,3);
    //sendRequest(adcSampleRequestSig,0);
    sendRequest(adcSampleRequestSig,1);
    sendRequest(adcSampleRequestSig,2);
    sendRequest(adcSampleRequestSig,3);
//    sendRequest(pcaChannelRequestSig);
//    sendRequestUbxMsgRates();
//    sendRequestGpioRateBuffer();
    sendRequest(temperatureRequestSig);
    sendRequest(i2cStatsRequestSig);
//    sendRequest(calibRequestSig);
}

void MainWindow::on_ipButton_clicked()
{
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
	QString portString;
    if (ipAndPort.size() == 2) {
		portString = ipAndPort.at(1);
	}
	else {
		portString = "51508";
    }
    makeConnection(ipAddress, portString.toUInt());
    if (!ui->ipBox->currentText().isEmpty() && ui->ipBox->findText(ui->ipBox->currentText()) == -1) {
		// if text not already in there, put it in there
		ui->ipBox->addItem(ui->ipBox->currentText());
	}
}

void MainWindow::on_discr1Slider_sliderPressed()
{
	mouseHold = true;
}

void MainWindow::on_discr1Slider_sliderReleased()
{
	mouseHold = false;
	on_discr1Slider_valueChanged(ui->discr1Slider->value());
}

void MainWindow::on_discr1Edit_editingFinished()
{
	float value = parseValue(ui->discr1Edit->text());
	if (value < 0) {
		return;
	}
	ui->discr1Slider->setValue((int)(value * 2 + 0.5));
}

void MainWindow::on_discr1Slider_valueChanged(int value)
{
    float thresh0 = (float)(value / 2000.0);
	ui->discr1Edit->setText(QString::number((float)value / 2.0) + "mV");
    if (!mouseHold) {
        sendSetThresh(0, thresh0);
	}
}

void MainWindow::on_discr2Slider_sliderPressed()
{
	mouseHold = true;
}

void MainWindow::on_discr2Slider_sliderReleased()
{
	mouseHold = false;
	on_discr2Slider_valueChanged(ui->discr2Slider->value());
}

void MainWindow::on_discr2Edit_editingFinished()
{
	float value = parseValue(ui->discr2Edit->text());
	if (value < 0) {
		return;
	}
	ui->discr2Slider->setValue((int)(value * 2 + 0.5));
}

void MainWindow::on_discr2Slider_valueChanged(int value)
{
    float thresh1 =  (float)(value / 2000.0);
	ui->discr2Edit->setText(QString::number((float)(value / 2.0)) + "mV");
    if (!mouseHold) {
        sendSetThresh(1, thresh1);
	}
}
void MainWindow::setMaxThreshVoltage(float voltage){
    // we have 0.5 mV resolution so we have (int)(mVolts)*2 steps on the slider
    // the '+0.5' is to round up or down like in mathematics
    maxThreshVoltage = voltage;
    int maximum = (int)(voltage*2000+0.5);
    ui->discr1Slider->setMaximum(maximum);
    ui->discr2Slider->setMaximum(maximum);
    int bigger = (sliderValues.at(0)>sliderValues.at(1))?0:1;
    if( sliderValues.at(bigger) > maximum){
        sendSetThresh(bigger,voltage);
        if (sliderValues.at(!bigger) > maximum){
            sendSetThresh(!bigger,voltage);
        }
    }
}
float MainWindow::parseValue(QString text) {
	// ignores everything that is not a number or at least most of it
	QRegExp alphabetical = QRegExp("[a-z]+[A-Z]+");
	QRegExp specialCharacters = QRegExp(
		QString::fromUtf8("[\\-`~!@#\\$%\\^\\&\\*()_\\—\\+=|:;<>«»\\?/{}\'\"ß\\\\]+"));
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

void MainWindow::on_biasPowerButton_clicked()
{
    sendSetBiasStatus(!biasON);
}

void MainWindow::sendInputSwitch(int id) {
    TcpMessage tcpMessage(pcaChannelSig);
    *(tcpMessage.dStream) << (quint8)id;
    emit sendTcpMessage(tcpMessage);
    sendRequest(pcaChannelRequestSig);
}

void MainWindow::on_biasVoltageSlider_sliderReleased()
{
    mouseHold = false;
    on_biasVoltageSlider_valueChanged(ui->biasVoltageSlider->value());
}

void MainWindow::on_biasVoltageSlider_valueChanged(int value)
{
    if (!mouseHold)
    {
        double biasVoltage = (double)value/ui->biasVoltageSlider->maximum()*maxBiasVoltage;
        if (fabs(biasCalibSlope)<1e-5) return;
        double dacVoltage = (biasVoltage-biasCalibOffset)/biasCalibSlope;
        if (dacVoltage<0.) dacVoltage=0.;
        if (dacVoltage>3.3) dacVoltage=3.3;
        sendSetBiasVoltage(dacVoltage);
    }
    // equation:
    // UBias = c1*UDac + c0
    // (UBias - c0)/c1 = UDac
}

void MainWindow::on_biasVoltageSlider_sliderPressed()
{
    mouseHold=true;
}

void MainWindow::onCalibUpdated(const QVector<CalibStruct>& items)
{
    if (calib==nullptr) return;

    TcpMessage tcpMessage(calibSetSig);
    if (items.size()) {
        *(tcpMessage.dStream) << (quint8)items.size();
        for (int i=0; i<items.size(); i++) {
            *(tcpMessage.dStream) << items[i];
        }
        emit sendTcpMessage(tcpMessage);
    }

    uint8_t flags = calib->getCalibParameter("CALIB_FLAGS").toUInt();
    bool calibedBias = false;
    if (flags & CalibStruct::CALIBFLAGS_VOLTAGE_COEFFS) calibedBias=true;

    const QStandardItemModel *model = dynamic_cast<QStandardItemModel*>(ui->biasControlTypeComboBox->model());
    QStandardItem *item = model->item(1);

    item->setEnabled(calibedBias);
/*
    item->setFlags(disable ? item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled)
                           : Qt::ItemIsSelectable|Qt::ItemIsEnabled));
    // visually disable by greying out - works only if combobox has been painted already and palette returns the wanted color
    item->setData(disable ? ui->comboBox->palette().color(QPalette::Disabled, QPalette::Text)
                          : QVariant(), // clear item data in order to use default color
                  Qt::TextColorRole);
*/
    ui->biasControlTypeComboBox->setCurrentIndex((calibedBias)?1:0);
//    sendRequest(biasVoltageRequestSig);
}

void MainWindow::on_biasControlTypeComboBox_currentIndexChanged(int index)
{
    if (index==1) {
        if (calib==nullptr) return;
        QString str = calib->getCalibParameter("COEFF0");
        if (!str.size()) return;
        double c0 = str.toDouble();
        str = calib->getCalibParameter("COEFF1");
        if (!str.size()) return;
        double c1 = str.toDouble();
        biasCalibOffset=c0; biasCalibSlope=c1;
        minBiasVoltage=0.; maxBiasVoltage=40.;
        ui->biasVoltageDoubleSpinBox->setMaximum(maxBiasVoltage);
        ui->biasVoltageDoubleSpinBox->setSingleStep(0.1);
    } else {
        biasCalibOffset=0.; biasCalibSlope=1.;
        minBiasVoltage=0.; maxBiasVoltage=3.3;
        ui->biasVoltageDoubleSpinBox->setMaximum(maxBiasVoltage);
        ui->biasVoltageDoubleSpinBox->setSingleStep(0.01);
    }
    sendRequest(biasVoltageRequestSig);
}

void MainWindow::on_biasVoltageDoubleSpinBox_valueChanged(double arg1)
{
    double biasVoltage = arg1;
    if (fabs(biasCalibSlope)<1e-5) return;
    double dacVoltage = (biasVoltage-biasCalibOffset)/biasCalibSlope;
    if (dacVoltage<0.) dacVoltage=0.;
    if (dacVoltage>3.3) dacVoltage=3.3;
    sendSetBiasVoltage(dacVoltage);
}
