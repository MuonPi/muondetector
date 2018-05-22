#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../shared/tcpconnection.h"
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
    ui->uartBuffer->setValue(0);
    ui->uartBuffer->setDisabled(true);
    // addressColumn = new QList<QStandardItem *>;
    //addressColumn->append(new QStandardItem("localhost"));
    //addressColumn->append(new QStandardItem("134.176.17.217"));
    addresses = new QStandardItemModel();
    //addresses->appendColumn(*addressColumn);
    loadSettings("ipAddresses.save",addresses);
    ui->ipBox->setModel(addresses);
    //ui->ipBox->installEventFilter(this);
    QPalette palette = ui->ipStatusLabel->palette();
    palette.setColor(ui->ipStatusLabel->foregroundRole(),Qt::darkGray);
    ui->ipStatusLabel->setPalette(palette);
    ui->ipBox->installEventFilter(this);
    ui->ipButton->installEventFilter(this);
}

void MainWindow::makeConnection(QString ipAddress){

    // if connection was successfully made: set "connectedToDemon" to true!
    connectedToDemon = true;
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
    }
    else
        return false;

}

MainWindow::~MainWindow()
{
    saveSettings(QString("ipAddresses.save"),addresses);
    delete ui;
}

void MainWindow::on_ipButton_clicked()
{
    QPalette palette;
    if (connectedToDemon){
        // it is connected and the button shows "disconnect" -> here comes disconnect code
        connectedToDemon = false;
        // set button and color of label
        palette = ui->ipStatusLabel->palette();
        palette.setColor(ui->ipStatusLabel->foregroundRole(),Qt::darkGray);
        ui->ipStatusLabel->setPalette(palette);
        ui->ipStatusLabel->setText("not connected");
        ui->ipButton->setText("connect");
        ui->ipBox->setEnabled(true);
        return;
    }
    makeConnection(ui->ipBox->currentText());
    if (!connectedToDemon){
        // after "makeConnection" it should be connected, if not: some error
        return;
    }
    if (!ui->ipBox->currentText().isEmpty()&&ui->ipBox->findText(ui->ipBox->currentText())==-1){
        // if text not already in there, put it in there
        ui->ipBox->addItem(ui->ipBox->currentText());
    }

    // change color and text of label and button
    palette = ui->ipStatusLabel->palette();
    palette.setColor(ui->ipStatusLabel->foregroundRole(),Qt::darkGreen);
    ui->ipStatusLabel->setPalette(palette);
    ui->ipStatusLabel->setText("connected");
    ui->ipButton->setText("disconnect");
    ui->ipBox->setDisabled(true);
}
