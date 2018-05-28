#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <tcpconnection.h>
#include <QStandardItemModel>
#include "../shared/tcpconnection.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void makeConnection(QString ipAddress);

private slots:
    void on_ipButton_clicked();
    void connected();

private:
    Ui::MainWindow *ui;
    int verbose = 0;
    TcpConnection *tcpConnection = nullptr;
    QStandardItemModel *addresses;
    QList<QStandardItem *> *addressColumn;
    bool saveSettings(QString fileName, QStandardItemModel* model);
    bool loadSettings(QString fileName, QStandardItemModel* model);
    bool eventFilter(QObject *object, QEvent *event);
    bool connectedToDemon = false;
};

#endif // MAINWINDOW_H
