#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <tcpconnection.h>
#include <QStandardItemModel>
#include <QErrorMessage>
#include <QTime>

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
	void addUbxMsgRates(QMap<uint16_t, int> ubxMsgRates);
	void sendTcpMessage(TcpMessage tcpMessage);
    void closeConnection();

public slots:
	void receivedTcpMessage(TcpMessage tcpMessage);
    void receivedGpioRisingEdge(quint8 pin, quint32 tick);
	void sendSetI2CProperties(I2cProperty i2cProperty);
	void requestI2CProperties();
	void requestUbxMsgRates();
	void stoppedConnection(QString remotePeerAddress, quint16 remotePeerPort, QString localAddress, quint16 localPort,
		quint32 timeoutTime, quint32 connectionDuration);
	void makeConnection(QString ipAddress, quint16 port);
	void updateI2CProperties(I2cProperty i2cProperty);

private slots:
	void updateUiProperties(bool bias_powerOn, int uartBufferValue = -1, int discr1SliderValue = -1,
		int discr2SliderValue = -1);
	// only those properties with value >= 0 will be updated!
	void resetAndHit();
	void resetXorHit();
	void on_ipButton_clicked();
	void connected();

	void on_discr1Edit_editingFinished();

	void on_discr1Slider_sliderReleased();

	void on_discr1Slider_valueChanged(int value);

	void on_discr1Slider_sliderPressed();

	void on_discr2Slider_sliderReleased();

	void on_discr2Slider_valueChanged(int value);

	void on_discr2Slider_sliderPressed();

	void on_biasPowerButton_clicked();

	void on_discr2Edit_editingFinished();

	void settings_clicked(bool checked);

private:
	Ui::MainWindow *ui;
	void uiSetConnectedState();
	void uiSetDisconnectedState();
	float parseValue(QString text);
	int verbose = 0;
	bool biasPowerOn = false;
	QErrorMessage errorM;
	TcpConnection *tcpConnection = nullptr;
	QStandardItemModel *addresses;
	QList<QStandardItem *> *addressColumn;
	bool saveSettings(QString fileName, QStandardItemModel* model);
	bool loadSettings(QString fileName, QStandardItemModel* model);
	bool eventFilter(QObject *object, QEvent *event);
	bool connectedToDemon = false;
	bool mouseHold = false;
	QTimer andTimer, xorTimer;
};

#endif // MAINWINDOW_H
