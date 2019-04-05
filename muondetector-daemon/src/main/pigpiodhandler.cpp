#include <pigpiodhandler.h>
#include <QDebug>
#include <gpio_pin_definitions.h>
#include <gpio_mapping.h>
#include <exception>
#include <iostream>
#include <QPointer>

extern "C" {
#include <pigpiod_if2.h>
}


const static int eventCountDeadTime = 30;
const static int adcSampleDeadTime = 10;

static int pi = 0;
static QPointer<PigpiodHandler> pigHandlerAddress; // QPointer automatically clears itself if pigHandler object is destroyed

static void cbFunction(int user_pi, unsigned int user_gpio,
    unsigned int level, uint32_t tick) {
    if (pigHandlerAddress.isNull()) {
        pigpio_stop(pi);
        return;
    }
    static uint32_t lastTick=0;
    QPointer<PigpiodHandler> pigpioHandler = pigHandlerAddress;
    try{
/*
        if (user_gpio == GPIO_PINMAP[ADC_READY]) {
//			std::cout<<"ADC conv ready"<<std::endl;
            return;
        }
*/
        if (user_gpio == GPIO_PINMAP[TIMEPULSE]) {
//			std::cout<<"Timepulse"<<std::endl;
            struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
            qint32 t_diff_us=ts.tv_nsec/1000;
            if (t_diff_us>500000L) t_diff_us=t_diff_us-1000000L;
            emit pigpioHandler->timePulseDiff(t_diff_us);
            return;
        }
        QDateTime now = QDateTime::currentDateTimeUtc();
        //qDebug()<<"gpio evt: gpio="<<user_gpio<<"  GPIO_PINMAP[EVT_XOR]="<<GPIO_PINMAP[EVT_XOR];
        if (user_gpio == GPIO_PINMAP[EVT_AND] || user_gpio == GPIO_PINMAP[EVT_XOR]){
            if (pigpioHandler->lastSamplingTime.msecsTo(now)>=adcSampleDeadTime) {
                emit pigpioHandler->samplingTrigger();
                pigpioHandler->lastSamplingTime = now;
            }
            quint64 nsecsElapsed=pigpioHandler->elapsedEventTimer.nsecsElapsed();
			pigpioHandler->elapsedEventTimer.start();
			//emit pigpioHandler->eventInterval(nsecsElapsed);
			emit pigpioHandler->eventInterval((tick-lastTick)*1000);
			lastTick=tick;
			
        }

        if (pi != user_pi) {
            // put some error here for the case pi is not the same as before initialized
        }
        // level gives the information if it is up or down (only important if trigger is
        // at both: rising and falling edge)
        emit pigpioHandler->signal(user_gpio);
    }
    catch (std::exception& e)
    {
        pigpioHandler = 0;
        pigpio_stop(pi);
        qDebug() << "Exception catched : " << e.what();
    }
}

PigpiodHandler::PigpiodHandler(QVector<unsigned int> gpio_pins, QObject *parent)
	: QObject(parent)
{
    startOfProgram = QDateTime::currentDateTimeUtc();
    lastSamplingTime = startOfProgram;
    elapsedEventTimer.start();
    pigHandlerAddress = this;
    pi = pigpio_start((char*)"127.0.0.1", (char*)"8888");
    if (pi < 0) {
        return;
    }
    for (auto& gpio_pin : gpio_pins) {
        set_mode(pi, gpio_pin, PI_INPUT);
        if (gpio_pin==GPIO_PINMAP[ADC_READY]) set_pull_up_down(pi, gpio_pin, PI_PUD_UP);
        callback(pi, gpio_pin, RISING_EDGE, cbFunction);
//        callback(pi, gpio_pin, FALLING_EDGE, cbFunction);
        //        if (value==pigif_bad_malloc||
        //            value==pigif_dublicate_callback||
        //            value==pigif_bad_callback){
        //            continue;
        //        }
    }
    isInitialised = true;
}

void PigpiodHandler::setInput(unsigned int gpio) {
    if (isInitialised) set_mode(pi, gpio, PI_INPUT);
}

void PigpiodHandler::setOutput(unsigned int gpio) {
    if (isInitialised) set_mode(pi, gpio, PI_OUTPUT);
}

void PigpiodHandler::setPullUp(unsigned int gpio) {
    if (isInitialised) set_pull_up_down(pi, gpio, PI_PUD_UP);
}

void PigpiodHandler::setPullDown(unsigned int gpio) {
    if (isInitialised) set_pull_up_down(pi, gpio, PI_PUD_DOWN);
}

void PigpiodHandler::setGpioState(unsigned int gpio, bool state) {
    if (isInitialised) {
        gpio_write(pi, gpio, (state)?1:0);
    }
}

bool PigpiodHandler::initialised(){
    return isInitialised;
}

void PigpiodHandler::stop() {
    if (!isInitialised){
        return;
    }
    isInitialised=false;
	pigpio_stop(pi);
    pigHandlerAddress.clear();
    this->deleteLater();
}
