#ifndef PIGPIODHANDLER_H
#define PIGPIODHANDLER_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QElapsedTimer>

#include <gpio_pin_definitions.h>

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
    // can't make it private because of access of PigpiodHandler with global pointer
    QDateTime startOfProgram, lastSamplingTime; // the exact time when the program starts (Utc)
    QElapsedTimer elapsedEventTimer;
	GPIO_PIN samplingTriggerSignal=EVT_XOR;
	
signals:
    void signal(uint8_t gpio_pin);
    void samplingTrigger();
	void eventInterval(quint64 nsecs);
	void timePulseDiff(qint32 usecs);

public slots:
	void stop();
    bool initialised();
    void setInput(unsigned int gpio);
    void setOutput(unsigned int gpio);
    void setPullUp(unsigned int gpio);
    void setPullDown(unsigned int gpio);
    void setGpioState(unsigned int gpio, bool state);
    void setSamplingTriggerSignal(GPIO_PIN signalName) { samplingTriggerSignal=signalName; }
private:
    bool isInitialised = false;
};


#endif // PIGPIODHANDLER_H
