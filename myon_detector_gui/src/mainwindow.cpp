#include <mainwindow.h>
#include <ui_mainwindow.h>
#include <QThread>
#include <QFile>
#include <QKeyEvent>
#include <QDebug>
#include <QErrorMessage>
#include <settings.h>
#include <status.h>
#include <tcpmessage_keys.h>
#include <map.h>
#include <gpio_pin_definitions.h>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
    qRegisterMetaType<TcpMessage>("TcpMessage");
    qRegisterMetaType<GeodeticPos>("GeodeticPos");
    ui->setupUi(this);
	ui->discr1Layout->setAlignment(ui->discr1Slider, Qt::AlignHCenter);
	ui->discr2Layout->setAlignment(ui->discr2Slider, Qt::AlignHCenter); // aligns the slider in their vertical layout centered
    QIcon icon("myon.png");
	this->setWindowIcon(icon);

	// initialise all ui elements that will be inactive at start
	uiSetDisconnectedState();
    setMaxThreshVoltage(1.0);

    // setup ipBox and load addresses etc.
    addresses = new QStandardItemModel(this);
	loadSettings("ipAddresses.save", addresses);
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
        ratePollTimer.setInterval(3000);
        ratePollTimer.setSingleShot(false);
        connect(&ratePollTimer, &QTimer::timeout, this, &MainWindow::requestRates);
        ratePollTimer.start();
    }

    // set all tabs
    ui->tabWidget->removeTab(0);
    Status *status = new Status(this);
    ui->tabWidget->addTab(status,"status");
    Settings *settings = new Settings(this);
    connect(this, &MainWindow::setUiEnabledStates, settings, &Settings::onUiEnabledStateChange);
    ui->tabWidget->addTab(settings,"settings");
    Map *map = new Map(this);
    ui->tabWidget->addTab(map, "map");
    connect(this, &MainWindow::geodeticPos, map, &Map::onGeodeticPosReceived);
    connect(this, &MainWindow::addUbxMsgRates, settings, &Settings::addUbxMsgRates);
    connect(settings, &Settings::sendRequestUbxMsgRates, this, &MainWindow::sendRequestUbxMsgRates);
    connect(settings, &Settings::sendSetUbxMsgRateChanges, this, &MainWindow::sendSetUbxMsgRateChanges);
    //settings->show();
	// set menu bar actions
    //connect(ui->actionsettings, &QAction::triggered, this, &MainWindow::settings_clicked);
}

MainWindow::~MainWindow()
{
	emit closeConnection();
	saveSettings(QString("ipAddresses.save"), addresses);
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
	connect(tcpThread, &QThread::finished, tcpConnection, &TcpConnection::deleteLater);
	connect(tcpConnection, &TcpConnection::connected, this, &MainWindow::connected);
    connect(this, &MainWindow::closeConnection, tcpConnection, &TcpConnection::closeThisConnection);
    connect(this, &MainWindow::sendTcpMessage, tcpConnection, &TcpConnection::sendTcpMessage);
    connect(tcpConnection, &TcpConnection::receivedTcpMessage, this, &MainWindow::receivedTcpMessage);
	tcpThread->start();
}

bool MainWindow::saveSettings(QString fileName, QStandardItemModel *model) {
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly)) {
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

bool MainWindow::loadSettings(QString fileName, QStandardItemModel* model) {
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
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
        quint8 gpioPin;
        *(tcpMessage.dStream) >> gpioPin;
        receivedGpioRisingEdge(gpioPin);
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
        *(tcpMessage.dStream) >> biasVoltage;
        updateUiProperties();
        return;
    }
    if (msgID == biasSig){
        *(tcpMessage.dStream) >> biasON;
        updateUiProperties();
        return;
    }
    if (msgID == pcaChannelSig){
        *(tcpMessage.dStream) >> pcaPortMask;
        updateUiProperties();
        return;
    }
    if (msgID == gpioRateSig){
        quint8 whichRate;
        float rate;
        *(tcpMessage.dStream) >> whichRate >> rate;
        emit gpioRate(whichRate, rate);
        if (whichRate == 0){
            ui->rate1->setText(QString::number(rate,'g',3)+"/s");
        }
        if (whichRate == 1){
            ui->rate2->setText(QString::number(rate,'g',3)+"/s");
        }
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
}

void MainWindow::sendRequest(quint16 requestSig){
    TcpMessage tcpMessage(requestSig);
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
}

void MainWindow::sendSetBiasStatus(bool status){
    TcpMessage tcpMessage(biasSig);
    *(tcpMessage.dStream) << status;
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::sendSetThresh(uint8_t channel, float value){
    TcpMessage tcpMessage(threshSig);
    *(tcpMessage.dStream) << channel << value;
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::sendSetUbxMsgRateChanges(QMap<uint16_t, int> changes){
    TcpMessage tcpMessage(ubxMsgRate);
    *(tcpMessage.dStream) << changes;
    emit sendTcpMessage(tcpMessage);
}

void MainWindow::requestRates(){
    quint8 whichRate = 0;
    TcpMessage xorRateRequest(gpioRateRequestSig);
    *(xorRateRequest.dStream) << whichRate;
    emit sendTcpMessage(xorRateRequest);
    whichRate = 1;
    TcpMessage andRateRequest(gpioRateRequestSig);
    *(andRateRequest.dStream) << whichRate;
    emit sendTcpMessage(andRateRequest);
}

void MainWindow::receivedGpioRisingEdge(quint8 pin) {
	if (pin == EVT_AND) {
		ui->ANDHit->setStyleSheet("QLabel {color: white; background-color: darkGreen;}");
		andTimer.start();
	}
	if (pin == EVT_XOR) {
		ui->XORHit->setStyleSheet("QLabel {color: white; background-color: darkGreen;}");
		xorTimer.start();
	}
}

void MainWindow::resetAndHit() {
	ui->ANDHit->setStyleSheet("QLabel {color: white; background-color: darkRed;}");
}
void MainWindow::resetXorHit() {
	ui->XORHit->setStyleSheet("QLabel {color: white; background-color: darkRed;}");
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
	ui->ANDHit->setStyleSheet("QLabel {color: darkGray;}");
	ui->XORHit->setDisabled(true);
    ui->XORHit->setStyleSheet("QLabel {color: darkGray;}");
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

	ui->ANDHit->setEnabled(true);
	ui->ANDHit->setStyleSheet("QLabel {background-color: darkRed; color: white;}");
	ui->XORHit->setEnabled(true);
	ui->XORHit->setStyleSheet("QLabel {background-color: darkRed; color: white;}");
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
    saveSettings(QString("ipAddresses.save"), addresses);
    uiSetConnectedState();
    sendRequest(biasVoltageRequestSig);
    sendRequest(biasRequestSig);
    sendRequest(threshRequestSig);
    sendRequest(pcaChannelRequestSig);
    sendRequestUbxMsgRates();
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
