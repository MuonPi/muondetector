#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QThread>
#include <QFile>
#include <QKeyEvent>
#include <QDebug>

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
    ui->uartBuffer->setValue(0);
    ui->uartBuffer->setDisabled(true);
    ui->discr1Slider->setValue(0);
    ui->discr1Slider->setDisabled(true);
    ui->discr1Edit->clear();
    ui->discr1Edit->setDisabled(true);
    ui->discr2Slider->setValue(0);
    ui->discr2Slider->setDisabled(true);
    ui->discr2Edit->clear();
    ui->discr2Edit->setDisabled(true);

    // setup ipBox and load addresses etc.
    addresses = new QStandardItemModel();
    loadSettings("ipAddresses.save",addresses);
    ui->ipBox->setModel(addresses);
    ui->ipBox->setAutoCompletion(true);
    ui->ipBox->setEditable(true);
    //ui->ipBox->installEventFilter(this);

    // setup colors
    ui->ipStatusLabel->setStyleSheet("QLabel {color : darkGray;}");
    ui->discr1Hit->setStyleSheet("QLabel {background-color : darkRed;}");
    ui->discr2Hit->setStyleSheet("QLabel {background-color : darkRed;}");
    /*QPalette palette = ui->ipStatusLabel->palette();
    palette.setColor(ui->ipStatusLabel->foregroundRole(),Qt::darkGray);
    ui->ipStatusLabel->setPalette(palette);
    palette = ui->discr1Hit->palette();
    palette.setColor(ui->discr1Hit->backgroundRole(),Qt::darkRed);
    ui->discr1Hit->setPalette(palette);
    palette.setColor(ui->discr2Hit->backgroundRole(),Qt::darkRed);
    ui->discr2Hit->setPalette(palette);
*/
    // setup event filter
    ui->ipBox->installEventFilter(this);
    ui->ipButton->installEventFilter(this);
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
    connect(tcpConnection, &TcpConnection::i2CProperties, this, &MainWindow::updateI2CProperties);
    connect(this, &MainWindow::setI2CProperties, tcpConnection, &TcpConnection::sendI2CProperties);
    //connect(this, &Demon::sendMsg, tcpConnection, &TcpConnection::sendMsg);
    //connect(tcpConnection, &TcpConnection::stoppedConnection, this, &Demon::stoppedConnection);
    tcpThread->start();
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

MainWindow::~MainWindow()
{
    saveSettings(QString("ipAddresses.save"),addresses);
    delete ui;
}

void MainWindow::updateI2CProperties(quint16 pcaChann,  QVector<float> dacThresh,
                                     float biasVoltage, bool biasPowerOn, bool setProperties){
    if (!setProperties){
        updateUiProperties(0, (int)(1000*dacThresh.at(0)), (int)(1000*dacThresh.at(1)));
                // uartBufferValue to be replaced with the correct value
    }
}


void MainWindow::updateUiProperties(int uartBufferValue, int discr1SliderValue,
                                    int discr2SliderValue){
    if (!(uartBufferValue<0)){
        ui->uartBuffer->setEnabled(true);
        ui->uartBuffer->setValue(uartBufferValue);
    }
    if (!(discr1SliderValue<0)){
        ui->discr1Edit->setEnabled(true);
        ui->discr1Edit->setText(""+discr1SliderValue);
    }
    if (!(discr2SliderValue<0)){
        ui->discr2Edit->setEnabled(true);
        ui->discr2Edit->setText(""+discr1SliderValue);
    }
}

void MainWindow::connected(){
    connectedToDemon = true;

    // change color and text of label and button
    ui->ipStatusLabel->setStyleSheet("QLabel {color: darkGreen;}");
    ui->ipStatusLabel->setText("connected");
    ui->ipButton->setText("disconnect");
    ui->ipBox->setDisabled(true);
}

void MainWindow::on_ipButton_clicked()
{
    if (connectedToDemon){
        // it is connected and the button shows "disconnect" -> here comes disconnect code
        connectedToDemon = false;
        // set button and color of label
        emit closeConnection();
        ui->ipStatusLabel->setStyleSheet("QLabel {color: darkGray;}");
        ui->ipStatusLabel->setText("not connected");
        ui->ipButton->setText("connect");
        ui->ipBox->setEnabled(true);
        // disable all relevant objects
        ui->uartBuffer->setValue(0);
        ui->uartBuffer->setDisabled(true);
        ui->discr1Slider->setValue(0);
        ui->discr1Slider->setDisabled(true);
        ui->discr1Edit->clear();
        ui->discr1Edit->setDisabled(true);
        ui->discr2Slider->setValue(0);
        ui->discr2Slider->setDisabled(true);
        ui->discr2Edit->clear();
        ui->discr2Edit->setDisabled(true);
        return;
    }
    QString ipBoxText = ui->ipBox->currentText();
    QStringList ipAndPort= ipBoxText.split(':');
    if (ipAndPort.size()!=2){
        qDebug() << "error, size of ipAndPort not 2";
    }
    QString ipAddress = ipAndPort.at(0);
    QString portString = ipAndPort.at(1);
    makeConnection(ipAddress, portString.toUInt());
    if (!ui->ipBox->currentText().isEmpty()&&ui->ipBox->findText(ui->ipBox->currentText())==-1){
        // if text not already in there, put it in there
        ui->ipBox->addItem(ui->ipBox->currentText());
    }
}
