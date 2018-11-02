#ifndef PIGPIODHANDLER_H
#define PIGPIODHANDLER_H

#include <QObject>
#include <QVector>
#include <QTime>
#include <QQueue>

#define XOR_RATE 0
#define AND_RATE 1
#define COMBINED_RATE 2

void cbFunction(int user_pi, unsigned int user_gpio,
	unsigned int level, uint32_t tick);
static QVector<unsigned int> DEFAULT_VECTOR;
class PigpiodHandler : public QObject
{
	Q_OBJECT
public:
	explicit PigpiodHandler(QVector<unsigned int> gpio_pins = DEFAULT_VECTOR,
		QObject *parent = nullptr);
    void bufferIntervalActualisation();
    QQueue<int> xorCounts, andCounts;
    // can't make it private because of access of PigpiodHandler with global pointer
    QTime lastAndTime, lastXorTime, startOfProgram, lastInterval;
    float getRate(quint8 whichRate);
    void resetBuffer();
    void setBufferTime(int msecs);
    void setBufferResolution(int msecs);
    int getCurrentBufferTime();
signals:
    void signal(uint8_t gpio_pin);

public slots:
    void sendSignal(unsigned int gpio_pin, uint32_t tick);
	void stop();
    bool initialised();
private:
    //quint64 xorCounts, andCounts;
    bool isInitialised = false;
    int bufferMsecs = 1000*120; // 120 seconds
    int bufferResolution = 500; // 500 msecs resolution
};


#endif // PIGPIODHANDLER_H
