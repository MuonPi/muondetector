#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "../shared/tcpconnection.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->ipBox->clear();
}

MainWindow::~MainWindow()
{
    delete ui;
}
