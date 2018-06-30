#include <mainwindow.h>
#include <ui_mainwindow.h>
#include <QThread>
#include <QFile>
#include <QKeyEvent>
#include <QDebug>
#include <QErrorMessage>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->discr1Layout->setAlignment(ui->discr1Slider,Qt::AlignHCenter);
    ui->discr2Layout->setAlignment(ui->discr2Slider,Qt::AlignHCenter); // aligns the slider in their vertical layout centered
    QIcon icon("../myon.png");
    this->setWindowIcon(icon);

    // initialise all ui elements that will be inactive at start
    uiSetDisconnectedState();

    // setup ipBox and load addresses etc.
    addresses = new QStandardItemModel();
    loadSettings("ipAddresses.save",addresses);
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
}

MainWindow::~MainWindow()
{
    emit closeConnection();
    saveSettings(QString("ipAddresses.save"),addresses);
    delete ui;
}

void MainWindow::makeConnection(QString ipAddress, quint16 port){
    // add popup windows for errors!!!
    QThread *tcpThread = new QThread();
    if (!tcpConnection){
        delete(tcpConnection);
    }
    tcpConnection = new TcpConnection(ipAddress, port, verbose);
    tcpConnection->moveToThread(tcpThread);
    connect(tcpThread, &QThread::started, tcpConnection, &TcpConnection::makeConnection);
    connect(tcpThread, &QThread::finished, tcpConnection, &TcpConnection::deleteLater);
    connect(tcpThread, &QThread::finished, tcpThread, &QThread::deleteLater);
    //connect(this, &Demon::sendFile, tcpConnection, &TcpConnection::sendFile);
    connect(tcpConnection, &TcpConnection::connected, this, &MainWindow::connected);
    //connect(tcpConnection, &TcpConnection::error, this, &Demon::displaySocketError);
    //connect(tcpConnection, &TcpConnection::toConsole, this, &Demon::toConsole);
    connect(tcpConnection, &TcpConnection::connectionTimeout, this, &MainWindow::makeConnection);
    connect(this, &MainWindow::closeConnection, tcpConnection, &TcpConnection::closeConnection);
    connect(tcpConnection, &TcpConnection::stoppedConnection, this, &MainWindow::stoppedConnection);
    connect(tcpConnection, &TcpConnection::i2CProperties, this, &MainWindow::updateI2CProperties);
    connect(this, &MainWindow::setI2CProperties, tcpConnection, &TcpConnection::sendI2CProperties);
    connect(this, &MainWindow::requestI2CProperties, tcpConnection, &TcpConnection::sendI2CPropertiesRequest);
    //connect(this, &Demon::sendMsg, tcpConnection, &TcpConnection::sendMsg);
    //connect(tcpConnection, &TcpConnection::stoppedConnection, this, &Demon::stoppedConnection);
    tcpThread->start();
}

void MainWindow::stoppedConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort,
                                   quint32 timeoutTime, quint32 connectionDuration){
    connectedToDemon = false;
    //qDebug() << "received stoppedConnection";
    uiSetDisconnectedState();
}

bool MainWindow::saveSettings(QString fileName, QStandardItemModel *model){
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)){
        return false;
    }
    QDataStream stream(&file);
    qint32 n(model->rowCount());
    stream << n;
    for (int i = 0; i < n; i++){
        model->item(i)->write(stream);
    }
    file.close();
    return true;
}

bool MainWindow::loadSettings(QString fileName, QStandardItemModel* model){
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)){
        return false;
    }
    QDataStream stream(&file);
    qint32 n;
    stream >> n;
    for (int i=0; i<n; i++){
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
        if (combobox==ui->ipBox){
            if (ke->key() == Qt::Key_Delete){
                ui->ipBox->removeItem(ui->ipBox->currentIndex());
            }
        }
        if (ke->key() == Qt::Key_Escape){
            QCoreApplication::quit();
            //this->deleteLater();
        }
        if (ke->key() == Qt::Key_Enter){
            this->on_ipButton_clicked();
        }
        return false;
    }else{
        return false;
    }
}

void MainWindow::updateI2CProperties(I2cProperty i2cProperty, bool setProperties){
    if (!setProperties){
        biasPowerOn = i2cProperty.bias_powerOn;
        QVector<float> dacThresh = {i2cProperty.thresh1, i2cProperty.thresh2};
        updateUiProperties(i2cProperty.bias_powerOn, 0, (int)(1000*dacThresh.at(0)), (int)(1000*dacThresh.at(1)));
                // uartBufferValue to be replaced with the correct value
    }
}


void MainWindow::uiSetDisconnectedState(){
    // set button and color of label
    ui->ipStatusLabel->setStyleSheet("QLabel {color: darkGray;}");
    ui->ipStatusLabel->setText("not connected");
    ui->ipButton->setText("connect");
    ui->ipBox->setEnabled(true);
    // disable all relevant objects
    ui->uartBuffer->setValue(0);
    ui->uartBuffer->setDisabled(true);
    ui->discr1Label->setStyleSheet("QLabel {color: darkGray;}");
    ui->discr2Label->setStyleSheet("QLabel {color: darkGray;}");
    ui->bufferUsageLabel->setStyleSheet("QLabel {color: darkGray;}");
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
    ui->biasPowerLabel->setDisabled(true);
    ui->biasPowerLabel->setStyleSheet("QLabel {color: darkGray;}");
    ui->biasPowerButton->setDisabled(true);
}

void MainWindow::uiSetConnectedState(){
    // change color and text of labels and buttons
    ui->ipStatusLabel->setStyleSheet("QLabel {color: darkGreen;}");
    ui->ipStatusLabel->setText("connected");
    ui->ipButton->setText("disconnect");
    ui->ipBox->setDisabled(true);
    ui->discr1Label->setStyleSheet("QLabel {color: black;}");
    ui->discr2Label->setStyleSheet("QLabel {color: black;}");
    ui->bufferUsageLabel->setStyleSheet("QLabel {color: black;}");
}

void MainWindow::updateUiProperties(bool bias_powerOn, int uartBufferValue, int discr1SliderValue,
                                    int discr2SliderValue){
    if (!(uartBufferValue<0)){
        ui->uartBuffer->setEnabled(true);
        ui->uartBuffer->setValue(uartBufferValue);
    }
    if (!(discr1SliderValue<0)){
        ui->discr1Slider->setEnabled(true);
        ui->discr1Slider->setValue(discr1SliderValue);
        ui->discr1Edit->setEnabled(true);
        ui->discr1Edit->setText(QString::number(discr1SliderValue)+"mV");
    }
    if (!(discr2SliderValue<0)){
        ui->discr2Slider->setEnabled(true);
        ui->discr2Slider->setValue(discr2SliderValue);
        ui->discr2Edit->setEnabled(true);
        ui->discr2Edit->setText(QString::number(discr2SliderValue)+"mV");
    }
    ui->ANDHit->setEnabled(true);
    ui->ANDHit->setStyleSheet("QLabel {background-color: darkRed; color: white;}");
    ui->XORHit->setEnabled(true);
    ui->XORHit->setStyleSheet("QLabel {background-color: darkRed; color: white;}");
    ui->biasPowerButton->setEnabled(true);
    ui->biasPowerLabel->setEnabled(true);
    if(bias_powerOn){
        ui->biasPowerButton->setText("Disable Power");
        ui->biasPowerLabel->setText("Power ON");
        ui->biasPowerLabel->setStyleSheet("QLabel {background-color: darkGreen; color: white;}");
    }else{
        ui->biasPowerButton->setText("Enable Power");
        ui->biasPowerLabel->setText("Power OFF");
        ui->biasPowerLabel->setStyleSheet("QLabel {background-color: red; color: white;}");
    }
}

void MainWindow::connected(){
    connectedToDemon = true;
    uiSetConnectedState();
    emit requestI2CProperties();
}

void MainWindow::on_ipButton_clicked()
{
    if (connectedToDemon){
        // it is connected and the button shows "disconnect" -> here comes disconnect code
        connectedToDemon = false;
        emit closeConnection();
        uiSetDisconnectedState();
        return;
    }
    QString ipBoxText = ui->ipBox->currentText();
    QStringList ipAndPort= ipBoxText.split(':');
    if (ipAndPort.size()!=2){
        QString errorMess = "error, size of ipAndPort not 2";
        errorM.showMessage(errorMess);
    }
    QString ipAddress = ipAndPort.at(0);
    if (ipAddress == "local" || ipAddress == "localhost"){
        ipAddress = "127.0.0.1";
    }
    QString portString;
    if (ipAndPort.size()>1){
         portString = ipAndPort.at(1);
    }else{
        portString = "51508";
    }
    makeConnection(ipAddress, portString.toUInt());
    if (!ui->ipBox->currentText().isEmpty()&&ui->ipBox->findText(ui->ipBox->currentText())==-1){
        // if text not already in there, put it in there
        ui->ipBox->addItem(ui->ipBox->currentText());
    }
}

void MainWindow::on_discr1Edit_editingFinished()
{
    bool ok;
    int value = ui->discr1Edit->text().toInt(&ok);
    if (!ok){
        errorM.showMessage("failed to parse discr1Edit to float");
        return;
    }
    ui->discr1Slider->setValue(value);
}

void MainWindow::on_discr1Slider_sliderReleased()
{
    mouseHold = false;
    on_discr1Slider_valueChanged(ui->discr1Slider->value());
}

void MainWindow::on_discr1Slider_valueChanged(int value)
{
//    if (lastValueChanged->elapsed()<100){
//        return;
//    }else{
//        lastValueChanged->restart();
//        on_discr1Slider_sliderReleased();
//    }
    I2cProperty i2cProperty;
    i2cProperty.bias_powerOn = biasPowerOn;
    i2cProperty.thresh1 = (float)(value/1000.0);
    ui->discr1Edit->setText(QString::number(value)+"mV");
    if (!mouseHold){
        emit setI2CProperties(i2cProperty);
    }
    // must think about a solution where it get's sent in intervals of a few tenths of a second
    // but must send LAST value!!
}

void MainWindow::on_discr1Slider_sliderPressed()
{
    mouseHold = true;
}

void MainWindow::on_discr2Slider_sliderReleased()
{
    mouseHold = false;
    on_discr2Slider_valueChanged(ui->discr2Slider->value());
}

void MainWindow::on_discr2Slider_valueChanged(int value)
{
    I2cProperty i2cProperty;
    i2cProperty.bias_powerOn = biasPowerOn;
    i2cProperty.thresh2 = (float)(value/1000.0);
    ui->discr2Edit->setText(QString::number(value)+"mV");
    if (!mouseHold){
        emit setI2CProperties(i2cProperty);
    }
}

void MainWindow::on_discr2Slider_sliderPressed()
{
    mouseHold = true;
}

void MainWindow::on_biasPowerButton_clicked()
{
    I2cProperty i2cProperty;
    i2cProperty.bias_powerOn = !biasPowerOn;
    emit setI2CProperties(i2cProperty);
}
