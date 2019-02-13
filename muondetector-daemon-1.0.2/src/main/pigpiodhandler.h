#ifndef PIGPIODHANDLER_H
#define PIGPIODHANDLER_H

#include <QObject>
#include <QVector>
#include <QPointF>
#include <QTime>
#include <QQueue>
#include <QTimer>
#include <QDateTime>

#define XOR_RATE 0
#define AND_RATE 1
#define COMBINED_RATE 2

static QVector<unsigned int> DEFAULT_VECTOR;
class PigpiodHandler : public QObject
{
	Q_OBJECT
public:
	explicit PigpiodHandler(QVector<unsigned int> gpio_pins = DEFAULT_VECTOR,
		QObject *parent = nullptr);
    void bufferIntervalActualisation();
    QList<quint64> xorCounts, andCounts;
    // can't make it private because of access of PigpiodHandler with global pointer

    QDateTime lastInterval, lastAndTime, lastXorTime, lastSamplingTime;
    QDateTime startOfProgram; // the exact time when the program starts (Utc)
    QVector<QPointF> getBufferedRates(int number, quint8 whichRate); // get last <number> entries of
                                                                     // buffered rates. If 0: get all.
    void setBufferTime(int msecs);
    void setBufferResolution(int msecs);
    int getCurrentBufferTime();
signals:
    void signal(uint8_t gpio_pin);
    void samplingTrigger();

public slots:
    void resetBuffer();
    void sendSignal(unsigned int gpio_pin, uint32_t tick);
	void stop();
    bool initialised();
    void setInput(unsigned int gpio);
    void setOutput(unsigned int gpio);
    void setPullUp(unsigned int gpio);
    void setPullDown(unsigned int gpio);
    void setGpioState(unsigned int gpio, bool state);
private slots:
    void onBufferRatesTimer();
private:
    void resetCounts();
    //quint64 xorCounts, andCounts;
    float getRate(quint8 whichRate);
    QVector<QPointF> xorBufferedRates;
    QVector<QPointF> andBufferedRates;
    QTimer bufferRatesTimer;
    bool isInitialised = false;
    qint64 bufferMsecs = 1000*60; // 60 seconds
    qint64 bufferResolution = 500; // 500 msecs resolution
};


#endif // PIGPIODHANDLER_H
