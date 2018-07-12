#include <pigpiodhandler.h>
#include <QDebug>
#include <../shared/gpio_pin_definitions.h>
extern "C"{
#include <pigpiod_if2.h>
}

static int pi = 0;
static PigpiodHandler *pigHandlerAddress = nullptr;
PigpiodHandler::PigpiodHandler(QVector<unsigned int> gpio_pins, QObject *parent)
    : QObject(parent)
{
    lastAndTime.start();
    lastXorTime.start();
    pi = pigpio_start((char*)"127.0.0.1",(char*)"8888");
    pigHandlerAddress = this;
    for (auto& gpio_pin : gpio_pins){
        set_mode(pi, gpio_pin, PI_INPUT);
        callback(pi, gpio_pin, RISING_EDGE, cbFunction);
//        if (value==pigif_bad_malloc||
//            value==pigif_dublicate_callback||
//            value==pigif_bad_callback){
//            continue;
//        }
    }
}

void PigpiodHandler::stop(){
    pigpio_stop(pi);
    pigHandlerAddress = nullptr;
    this->deleteLater();
}
void PigpiodHandler::sendSignal(unsigned int gpio_pin, uint32_t tick){
    if (gpio_pin<256){
        emit signal((uint8_t)gpio_pin, tick);
    }
}
void cbFunction(int user_pi, unsigned int user_gpio,
                                unsigned int level, uint32_t tick){
    if (pigHandlerAddress==nullptr){
        return;
    }
    PigpiodHandler* pigpioHandler = pigHandlerAddress;
    if (user_gpio==EVT_AND){
        if(pigpioHandler->lastAndTime.elapsed()<40){
            return;
        }else{
            pigpioHandler->lastAndTime.restart();
        }
    }
    if (user_gpio==EVT_XOR){
        if(pigpioHandler->lastXorTime.elapsed()<40){
            return;
        }else{
            pigpioHandler->lastXorTime.restart();
        }
    }
    if (pi!=user_pi){
        // put some error here for the case pi is not the same as before initialized
    }
    // level gives the information if it is up or down (only important if trigger is
    // at both: rising and falling edge)
    pigpioHandler->sendSignal(user_gpio, tick);
}
