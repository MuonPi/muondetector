#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <tcpconnection.h>
#include <QStandardItemModel>
#include <QErrorMessage>
#include <QTime>
#include <geodeticpos.h>

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
    void gpioRate(quint8 whichrate, float rate);
    void tcpDisconnected();
    void setUiEnabledStates(bool enabled);
    void geodeticPos(GeodeticPos pos);

public slots:
	void receivedTcpMessage(TcpMessage tcpMessage);
    void receivedGpioRisingEdge(quint8 pin);
    void sendRequestUbxMsgRates();
    void sendSetUbxMsgRateChanges(QMap<uint16_t, int> changes);
	void makeConnection(QString ipAddress, quint16 port);

private slots:
	// only those properties with value >= 0 will be updated!
	void resetAndHit();
	void resetXorHit();
    void requestRates();
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

private:
	Ui::MainWindow *ui;
	void uiSetConnectedState();
	void uiSetDisconnectedState();
	float parseValue(QString text);
    void sendRequest(quint16 requestSig);
    void sendSetBiasVoltage(float voltage);
    void sendSetBiasStatus(bool status);
    void sendSetThresh(uint8_t channel, float value);
    void setMaxThreshVoltage(float voltage);
    void updateUiProperties();
    int verbose = 0;
    float biasVoltage = 0;
    bool biasON, uiValuesUpToDate = false;
    quint8 pcaPortMask = 0;
    QVector<int> sliderValues = QVector<int>({0,0});
    float maxThreshVoltage;
	QErrorMessage errorM;
	TcpConnection *tcpConnection = nullptr;
	QStandardItemModel *addresses;
	QList<QStandardItem *> *addressColumn;
	bool saveSettings(QString fileName, QStandardItemModel* model);
	bool loadSettings(QString fileName, QStandardItemModel* model);
	bool eventFilter(QObject *object, QEvent *event);
	bool connectedToDemon = false;
	bool mouseHold = false;
    bool automaticRatePoll = true;
    QTimer andTimer, xorTimer, ratePollTimer;
};

#endif // MAINWINDOW_H
