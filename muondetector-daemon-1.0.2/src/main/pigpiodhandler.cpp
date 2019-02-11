#include <pigpiodhandler.h>
#include <QDebug>
#include <gpio_pin_definitions.h>
#include <exception>
#include <iostream>
#include <QPointer>

extern "C" {
#include <pigpiod_if2.h>
}


const static int eventCountDeadTime = 30;
const static int adcSampleDeadTime = 10;

static int pi = 0;
const static int rateSecondsBuffered = 15*60; // 15 min
static QPointer<PigpiodHandler> pigHandlerAddress = 0; // QPointer automatically clears itself if pigHandler object is destroyed
PigpiodHandler::PigpiodHandler(QVector<unsigned int> gpio_pins, QObject *parent)
	: QObject(parent)
{
    startOfProgram = QDateTime::currentDateTimeUtc();
    lastAndTime = startOfProgram;
    lastXorTime = startOfProgram;
    lastInterval = startOfProgram;
    lastSamplingTime = startOfProgram;
    andCounts.push_front(0);
    xorCounts.push_front(0);
    pigHandlerAddress = this;
    pi = pigpio_start((char*)"127.0.0.1", (char*)"8888");
    if (pi < 0) {
        return;
    }
    for (auto& gpio_pin : gpio_pins) {
        set_mode(pi, gpio_pin, PI_INPUT);
        if (gpio_pin==ADC_READY) set_pull_up_down(pi, gpio_pin, PI_PUD_UP);
        callback(pi, gpio_pin, RISING_EDGE, cbFunction);
//        callback(pi, gpio_pin, FALLING_EDGE, cbFunction);
        //        if (value==pigif_bad_malloc||
        //            value==pigif_dublicate_callback||
        //            value==pigif_bad_callback){
        //            continue;
        //        }
    }
    isInitialised = true;
    connect(&bufferRatesTimer, &QTimer::timeout, this, &PigpiodHandler::onBufferRatesTimer);
    bufferRatesTimer.setSingleShot(false);
    bufferRatesTimer.setInterval(1000);
    bufferRatesTimer.start();
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

void PigpiodHandler::onBufferRatesTimer(){
    QPointF xorPoint(0.0, (qreal)getRate(XOR_RATE));
    QPointF andPoint(0.0, (qreal)getRate(AND_RATE));
    qint64 msecs = startOfProgram.msecsTo(QDateTime::currentDateTimeUtc());
    if (msecs <= 0){
        qDebug() << "the start-time of the program is in the future. Error.";
    }
    qreal x = ((qreal)msecs)/1000.0;//static_cast<qreal>(startOfProgram.msecsTo(QDateTime::currentDateTimeUtc()));
    // qreal is usually double or float (on armhf it should be float)
    xorPoint.setX(x);
    andPoint.setX(x);
    xorBufferedRates.push_back(xorPoint);
    andBufferedRates.push_back(andPoint);
    while (xorBufferedRates.first().x() < (x-(qreal)rateSecondsBuffered)){
        xorBufferedRates.removeFirst();
    }
    while (andBufferedRates.first().x() < (x-(qreal)rateSecondsBuffered)){
        andBufferedRates.removeFirst();
    }
}

QVector<QPointF> PigpiodHandler::getBufferedRates(int number, quint8 whichRate){
    QVector<QPointF> someRates;
    //QVector<QPointF> *bufferedRates;
    if (whichRate == XOR_RATE){
        if (number==0){
            return xorBufferedRates;
        }
        if (number>=xorBufferedRates.size()){
            return xorBufferedRates;
        }
        for (int i = xorBufferedRates.size()-number; i < xorBufferedRates.size(); i++){
            if (i>=0){
                someRates.push_back(xorBufferedRates.at(i));
            }
        }
    }
    if (whichRate == AND_RATE){
        if (number==0){
            return andBufferedRates;
        }
        if (number>=andBufferedRates.size()){
            return andBufferedRates;
        }
        for (int i = andBufferedRates.size()-number; i < andBufferedRates.size(); i++){
            if (i>=0){
                someRates.push_back(andBufferedRates.at(i));
            }
        }
    }
    return someRates;
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
	pigHandlerAddress = nullptr;
    this->deleteLater();
}

// internal functions:

void PigpiodHandler::sendSignal(unsigned int gpio_pin, uint32_t tick) {
	if (gpio_pin < 256) {
        emit signal((uint8_t)gpio_pin);
	}
}

void PigpiodHandler::resetBuffer(){
    resetCounts();
    andBufferedRates.clear();
    xorBufferedRates.clear();
    bufferRatesTimer.start();
}

void PigpiodHandler::resetCounts(){
    lastInterval = QDateTime::currentDateTimeUtc();
    andCounts.clear();
    andCounts.push_front(0);
    xorCounts.clear();
    xorCounts.push_front(0);
}
void PigpiodHandler::setBufferTime(int msecs){
    bufferMsecs = msecs;
    resetCounts();
}
void PigpiodHandler::setBufferResolution(int msecs){
    bufferResolution = msecs;
    resetCounts();
}
int PigpiodHandler::getCurrentBufferTime(){
    // returns the time in msecs that the buffer had time to build up since last reset
    // up to a maximum of 'bufferMsecs' defined in pigpiodhandler.h or set by 'setBufferTime(int msecs)'
    int numberOfEntries = xorCounts.size();
    if (numberOfEntries != andCounts.size()){
        qDebug() << "size of and-/xor-counters not equal but this should be prevented by bufferIntervalActualisation";
        while (xorCounts.size() > andCounts.size()){
            xorCounts.removeLast();
        }
        while (xorCounts.size() < andCounts.size()){
            andCounts.removeLast();
        }
        numberOfEntries = xorCounts.size();
    }
    return (numberOfEntries*bufferResolution-bufferResolution+lastInterval.msecsTo(QDateTime::currentDateTimeUtc()));
}
float PigpiodHandler::getRate(quint8 whichRate){
    bufferIntervalActualisation();
    float someRate;
    quint64 counts = 0;
    if (whichRate==XOR_RATE || whichRate==COMBINED_RATE){
        for (auto someCounts : xorCounts){
            counts += someCounts;
        }
    }
    if (whichRate==AND_RATE || whichRate==COMBINED_RATE){
        for (auto someCounts : andCounts){
            counts += someCounts;
        }
    }
    someRate = (float)counts*1000.0/((float)getCurrentBufferTime());
    // this rate should be accurate although we may have to check if it makes sense
    // we also have to consider using another way of time measurement since in this way
    // we get a problem if the last event is more than 24 Hours from currentTime
    return someRate;
}
void PigpiodHandler::bufferIntervalActualisation(){
    while (lastInterval.msecsTo(QDateTime::currentDateTimeUtc()) > bufferResolution){
        if (andCounts.isEmpty() || xorCounts.isEmpty()){
            qDebug() << "error, queue is empty. This should never happen";
            return;
        }
        while (andCounts.size()>=bufferMsecs/bufferResolution){
            andCounts.removeLast();
        }
        andCounts.push_front(0);
        while (xorCounts.size()>=bufferMsecs/bufferResolution){
            xorCounts.removeLast();
        }
        xorCounts.push_front(0);
        lastInterval = lastInterval.addMSecs(bufferResolution);
    }
}

void cbFunction(int user_pi, unsigned int user_gpio,
	unsigned int level, uint32_t tick) {
    if (pigHandlerAddress.isNull()) {
        pigpio_stop(pi);
        return;
    }
    QPointer<PigpiodHandler> pigpioHandler = pigHandlerAddress;
    QDateTime now = QDateTime::currentDateTimeUtc();
    try{
		if (user_gpio == ADC_READY) {
//			std::cout<<"ADC conv ready"<<std::endl;
			return;
        }
        if (user_gpio == TIMEPULSE) {
//			std::cout<<"Timepulse"<<std::endl;
			//return;
        }
        if (user_gpio == EVT_AND || user_gpio == EVT_XOR){
            if (pigpioHandler->lastSamplingTime.msecsTo(now)>=adcSampleDeadTime) {
                pigpioHandler->samplingTrigger();
                pigpioHandler->lastSamplingTime = now;
            }
        }
        if (user_gpio == EVT_AND) {
			pigpioHandler->bufferIntervalActualisation();
            if (!pigpioHandler->andCounts.isEmpty()){
                pigpioHandler->andCounts.first() = pigpioHandler->andCounts.first() +1;
            }
            if (pigpioHandler->lastAndTime.msecsTo(now) < eventCountDeadTime) {
                return;
            }
            else {
                pigpioHandler->lastAndTime = now;
            }
        }
        if (user_gpio == EVT_XOR) {
			pigpioHandler->bufferIntervalActualisation();
            if (!pigpioHandler->xorCounts.isEmpty()){
                pigpioHandler->xorCounts.first() = pigpioHandler->xorCounts.first() +1;
            }
            if (pigpioHandler->lastXorTime.msecsTo(now) < eventCountDeadTime) {
                return;
            }
            else {
                pigpioHandler->lastXorTime = now;
            }
            //std::cout<<"XOR event"<<std::endl;
        } 

        if (pi != user_pi) {
            // put some error here for the case pi is not the same as before initialized
        }
        // level gives the information if it is up or down (only important if trigger is
        // at both: rising and falling edge)
        pigpioHandler->sendSignal(user_gpio, tick);
    }
    catch (std::exception& e)
    {
        pigpioHandler = 0;
        pigpio_stop(pi);
        qDebug() << "Exception catched : " << e.what();
    }
}
