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

signals:
    void closeConnection();

public slots:
    void makeConnection(QString ipAddress, quint16 port);

private slots:
    void updateUiProperties(int uartBufferValue = -1, int discr1SliderValue = -1,
                                        int discr2SliderValue = -1);
    // only those properties with value >= 0 will be updated!
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
