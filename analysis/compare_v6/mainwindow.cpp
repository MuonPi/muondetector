#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    model = new QFileSystemModel();
    workDirectory = QFileDialog::getExistingDirectory(this, tr("Open Work Directory"),
                                                                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                                QFileDialog::ShowDirsOnly|QFileDialog::DontConfirmOverwrite);
    ui->dirView->setModel(model);
    if(!workDirectory.isEmpty()){
        model->setRootPath(workDirectory);
        QModelIndex idx = model->index(workDirectory);
        ui->dirView->setRootIndex(idx);
    }
    ui->dirView->setWindowTitle(QObject::tr("Dir View"));
    ui->dirView->hideColumn(2);
    //for (int i = 0; i < 4; i++){
    //    ui->dirView->resizeColumnToContents(i);
    //}
    int width = this->width()/2;
    qDebug() << width;
    ui->dirView->setColumnWidth(0,width/2);
    qDebug() << ui->dirView->columnWidth(0);
    ui->dirView->setColumnWidth(1,width/4-22);
    ui->dirView->setColumnWidth(3,width/4);
}

MainWindow::~MainWindow()
{
    delete ui;
}
