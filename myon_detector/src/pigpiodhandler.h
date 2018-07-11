#ifndef PIGPIODHANDLER_H
#define PIGPIODHANDLER_H

#include <QObject>
#include <QVector>

void cbFunction(int user_pi, unsigned int user_gpio,
                       unsigned int level, uint32_t tick);
static QVector<unsigned int> DEFAULT_VECTOR;
class PigpiodHandler : public QObject
{
    Q_OBJECT
public:
    explicit PigpiodHandler(QVector<unsigned int> gpio_pins = DEFAULT_VECTOR,
                            QObject *parent = nullptr);

signals:
    void signal(uint8_t gpio_pin, uint32_t tick);

public slots:
    void sendSignal(unsigned int gpio_pin, uint32_t tick);
    void stop();
private:
};


#endif // PIGPIODHANDLER_H
