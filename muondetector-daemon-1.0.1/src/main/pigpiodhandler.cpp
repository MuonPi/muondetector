#include <pigpiodhandler.h>
#include <QDebug>
#include <gpio_pin_definitions.h>
#include <exception>
extern "C" {
#include <pigpiod_if2.h>
}

static int pi = 0;
static PigpiodHandler *pigHandlerAddress = nullptr;
PigpiodHandler::PigpiodHandler(QVector<unsigned int> gpio_pins, QObject *parent)
	: QObject(parent)
{
    startOfProgram = QDateTime::currentDateTimeUtc();
    lastAndTime.start();
    lastXorTime.start();
    lastInterval = QTime::currentTime();
    andCounts.push_front(0);
    xorCounts.push_front(0);
    pi = pigpio_start((char*)"127.0.0.1", (char*)"8888");
    if (pi < 0) {
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
    isInitialised = true;
    connect(&bufferRatesTimer, &QTimer::timeout, this, &PigpiodHandler::onBufferRatesTimer);
    bufferRatesTimer.setSingleShot(false);
    bufferRatesTimer.setInterval(1000);
    bufferRatesTimer.start();
}

void PigpiodHandler::onBufferRatesTimer(){
    QPointF xorPoint = getRate(XOR_RATE);
    QPointF andPoint = getRate(AND_RATE);
    qreal x = static_cast<qreal>(startOfProgram.msecsTo(QDateTime::currentDateTimeUtc()));
    // qreal is usually double or float (on armhf it should be float)
    xorPoint.setX(x);
    andPoint.setX(x);
    xorBufferedRates.push_front(xorPoint);
    andBufferedRates.push_front(andPoint);
    while (xorBufferedRates.size()>rateBufferMaxSize){
        xorBufferedRates.pop_back();
    }
    while (andBufferedRates.size()>rateBufferMaxSize){
        andBufferedRates.pop_back();
    }
}

QVector<QPointF> PigpiodHandler::getBufferedRates(int number, quint8 whichRate){
    QVector<QPointF> someRates;
    QVector<QPointF> *bufferedRates;
    if (whichRate == XOR_RATE){
        bufferedRates = &xorBufferedRates;
    }
    if (whichRate == AND_RATE){
        bufferedRates = &andBufferedRates;
    }
    if (bufferedRates == nullptr){
        return QVector<QPointF>();
    }
    if (number==0){
        return *bufferedRates;
    }
    if (number>bufferedRates->size()){
        number = bufferedRates->size();
    }
    for (int i = 0; i < number; i++){
        someRates.push_back(bufferedRates->at(i));
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
}

// internal functions:

void PigpiodHandler::sendSignal(unsigned int gpio_pin, uint32_t tick) {
	if (gpio_pin < 256) {
        emit signal((uint8_t)gpio_pin);
	}
}
void PigpiodHandler::resetBuffer(){
    lastInterval = QTime::currentTime();
    andCounts.clear();
    andCounts.push_front(0);
    xorCounts.clear();
    xorCounts.push_front(0);
}
void PigpiodHandler::setBufferTime(int msecs){
    bufferMsecs = msecs;
    resetBuffer();
}
void PigpiodHandler::setBufferResolution(int msecs){
    bufferResolution = msecs;
    resetBuffer();
}
int PigpiodHandler::getCurrentBufferTime(){
    // returns the time in msecs that the buffer had time to build up since last reset
    // up to a maximum of 'bufferMsecs' defined in pigpiodhandler.h or set by 'setBufferTime(int msecs)'
    int numberOfEntries = xorCounts.size();
    if (numberOfEntries != andCounts.size()){
        qDebug() << "error: size of xorCounts and andCounts are not same... this should not happen!";
    }
    return (numberOfEntries*bufferResolution-bufferResolution+lastInterval.msecsTo(QTime::currentTime()));
}
QPointF PigpiodHandler::getRate(quint8 whichRate){
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
    QPointF somePoint(0, someRate);
    return somePoint;
}
void PigpiodHandler::bufferIntervalActualisation(){
    if (lastInterval.msecsTo(QTime::currentTime())<0){
        // indication of one day lapsed...
        // dump all rate information and start again!
        resetBuffer();
    }
    while (lastInterval.msecsTo(QTime::currentTime()) > bufferResolution){
        if (andCounts.isEmpty() || xorCounts.isEmpty()){
            qDebug() << "error, queue is empty. This should never happen";
            return;
        }
        if (andCounts.size()>=bufferMsecs/bufferResolution){
            andCounts.pop_back();
        }
        andCounts.push_front(0);
        if (xorCounts.size()>=bufferMsecs/bufferResolution){
            xorCounts.pop_back();
        }
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
    try{
        pigpioHandler->bufferIntervalActualisation();
        if (user_gpio == EVT_AND) {
            if (!pigpioHandler->andCounts.isEmpty()){
                pigpioHandler->andCounts.head()++;
            }
            if (pigpioHandler->lastAndTime.elapsed() < 30) {
                return;
            }
            else {
                pigpioHandler->lastAndTime.restart();
            }
        }
        if (user_gpio == EVT_XOR) {
            if (!pigpioHandler->xorCounts.isEmpty()){
                pigpioHandler->xorCounts.head()++;
            }
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
    catch (std::exception& e)
    {
        pigpioHandler = nullptr;
        qDebug() << "Exception catched : " << e.what();
    }
}
