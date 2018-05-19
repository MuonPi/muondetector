#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../shared/tcpconnection.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->discr1Layout->setAlignment(ui->discr1Slider,Qt::AlignHCenter);
    ui->discr2Layout->setAlignment(ui->discr2Slider,Qt::AlignHCenter); // aligns the slider in their vertical layout centered
    QIcon icon("../myon.png");
    this->setWindowIcon(icon);
    ui->ipBox->clear();
    ui->uartBuffer->setValue(0);
    ui->uartBuffer->setDisabled(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}
