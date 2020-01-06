#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>
#include <selectionmodel.h>
#include <compare_v6.cpp>
#include <QTreeWidget>
#include <QAction>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    selectionModel = new SelectionModel();
    ui->selection->setModel(selectionModel);
    ui->selection->setDefaultDropAction(Qt::MoveAction);
    ui->selection->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->selection, &QTreeView::customContextMenuRequested, this, &MainWindow::customContextMenu);
    fileModel = new FileModel();
    workDirectory = QFileDialog::getExistingDirectory(this, tr("Open Work Directory"),
                                                                QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                                QFileDialog::ShowDirsOnly|QFileDialog::DontConfirmOverwrite);
    ui->dirView->setModel(fileModel);
    ui->dirView->setDefaultDropAction(Qt::CopyAction);
    if(!workDirectory.isEmpty()){
        fileModel->setRootPath(workDirectory);
        QModelIndex idx = fileModel->index(workDirectory);
        ui->dirView->setRootIndex(idx);
    }
    ui->dirView->setWindowTitle(QObject::tr("Dir View"));
    ui->dirView->hideColumn(2);
    //for (int i = 0; i < 4; i++){
    //    ui->dirView->resizeColumnToContents(i);
    //}
    int width = this->width()/2;
    ui->dirView->setColumnWidth(0,width/2);
    ui->dirView->setColumnWidth(1,width/4-22);
    ui->dirView->setColumnWidth(3,width/4);
}

void MainWindow::customContextMenu(const QPoint &pos){
    QTreeView *tree = dynamic_cast<QTreeView*>(ui->selection);

    //QModelIndex inx = tree->indexAt(pos);
    //QString path = dynamic_cast<FileModel*>(tree->model())->filePath(inx);

    //qDebug()<<pos<<path;


    QAction *newAct = new QAction(QIcon(":/Resource/warning32.ico"), tr("&New"), this);
    newAct->setStatusTip(tr("new detector"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newDev()));


    QMenu menu(this);
    menu.addAction(newAct);

    QPoint pt(pos);
    menu.exec( tree->mapToGlobal(pos) );
}

MainWindow::~MainWindow()
{
    delete ui;
}
