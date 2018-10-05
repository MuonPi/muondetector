#include <pigpiodhandler.h>
#include <QDebug>
#include <../shared/gpio_pin_definitions.h>
extern "C" {
#include <pigpiod_if2.h>
}

static int pi = 0;
static PigpiodHandler *pigHandlerAddress = nullptr;
PigpiodHandler::PigpiodHandler(QVector<unsigned int> gpio_pins, QObject *parent)
	: QObject(parent)
{
	lastAndTime.start();
	lastXorTime.start();
    for (quint64 i = 0; i < bufferMsecs/bufferResolution; i++){
        xorCounts.push_back(0);
        andCounts.push_back(0);
    }
    startOfProgram = QTime::currentTime();
	pi = pigpio_start((char*)"127.0.0.1", (char*)"8888");
	if (pi < 0) {
		this->deleteLater();
		return;
	}
	pigHandlerAddress = this;
	for (auto& gpio_pin : gpio_pins) {
		set_mode(pi, gpio_pin, PI_INPUT);
		callback(pi, gpio_pin, RISING_EDGE, cbFunction);
		//        if (value==pigif_bad_malloc||
		//            value==pigif_dublicate_callback||
		//            value==pigif_bad_callback){
		//            continue;
		//        }
	}
}

void PigpiodHandler::stop() {
	pigpio_stop(pi);
	pigHandlerAddress = nullptr;
	this->deleteLater();
}
void PigpiodHandler::sendSignal(unsigned int gpio_pin, uint32_t tick) {
	if (gpio_pin < 256) {
        emit signal((uint8_t)gpio_pin);
	}
}
float PigpiodHandler::getRate(uint8_t whichRate){
    bufferIntervalActualisation();
    float someRate;
    quint64 counts = 0;
    if (whichRate==XOR_RATE){
        for (auto someCounts : xorCounts){
            counts += someCounts;
        }
    }
    if (whichRate==AND_RATE){
        for (auto someCounts : andCounts){
            counts += someCounts;
        }
    }
    if (whichRate==COMBINED_RATE){
        for (auto someCounts : xorCounts){
            counts += someCounts;
        }
    }
    someRate = counts*1000.0/(bufferMsecs-bufferResolution+lastInterval.msecsTo(QTime::currentTime()));
    return someRate;
}
void PigpiodHandler::bufferIntervalActualisation(){
    while (lastInterval.msecsTo(QTime::currentTime()) > bufferResolution){
        andCounts.pop_back();
        andCounts.push_front(0);
        xorCounts.pop_back();
        xorCounts.push_front(0);
        lastInterval = lastInterval.addMSecs(bufferResolution);
    }
}
void cbFunction(int user_pi, unsigned int user_gpio,
	unsigned int level, uint32_t tick) {
	if (pigHandlerAddress == nullptr) {
		return;
	}
	PigpiodHandler* pigpioHandler = pigHandlerAddress;
    pigpioHandler->bufferIntervalActualisation();
	if (user_gpio == EVT_AND) {
        pigpioHandler->andCounts.head()++;
        if (pigpioHandler->lastAndTime.elapsed() < 30) {
			return;
		}
		else {
			pigpioHandler->lastAndTime.restart();
		}
	}
	if (user_gpio == EVT_XOR) {
        pigpioHandler->xorCounts.head()++;
        if (pigpioHandler->lastXorTime.elapsed() < 30) {
			return;
		}
		else {
			pigpioHandler->lastXorTime.restart();
		}
	}
	if (pi != user_pi) {
		// put some error here for the case pi is not the same as before initialized
	}
	// level gives the information if it is up or down (only important if trigger is
	// at both: rising and falling edge)
	pigpioHandler->sendSignal(user_gpio, tick);
}
