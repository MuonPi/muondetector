#include <pigpiodhandler.h>
#include <QDebug>
#include <gpio_mapping.h>
#include <exception>
#include <iostream>
#include <QPointer>

extern "C" {
#include <pigpiod_if2.h>
}


const static int eventCountDeadTime = 50;
const static int adcSampleDeadTime = 8;

static int pi = -1;
static int spiHandle = -1;
static QPointer<PigpiodHandler> pigHandlerAddress; // QPointer automatically clears itself if pigHandler object is destroyed

static void cbFunction(int user_pi, unsigned int user_gpio,
    unsigned int level, uint32_t tick) {
    //qDebug() << "callback user_pi: " << user_pi << " user_gpio: " << user_gpio << " level: "<< level << " pigHandlerAddressNull: " << pigHandlerAddress.isNull() ;
    if (pigHandlerAddress.isNull()) {
        pigpio_stop(pi);
        return;
    }
    static uint32_t lastTick=0;
    QPointer<PigpiodHandler> pigpioHandler = pigHandlerAddress;

    if (user_gpio == 20){
        //qDebug()<< "emit signal for pin 20";
        emit pigpioHandler->signal((uint8_t)user_gpio);
        return;
    }

    try{
		// allow only registered signals to be processed here
		// if gpio pin fired which is not in GPIO_PIN list: return
        auto it=std::find_if(GPIO_PINMAP.cbegin(), GPIO_PINMAP.cend(), [&user_gpio](const std::pair<GPIO_PIN, unsigned int>& val) {
			if (val.second==user_gpio) return true;
			return false;
		});
        if (it==GPIO_PINMAP.end()) return;

/*
        if (user_gpio == GPIO_PINMAP[ADC_READY]) {
//			std::cout<<"ADC conv ready"<<std::endl;
            return;
        }
*/
        QDateTime now = QDateTime::currentDateTimeUtc();
        //qDebug()<<"gpio evt: gpio="<<user_gpio<<"  GPIO_PINMAP[EVT_XOR]="<<GPIO_PINMAP[EVT_XOR];
//        if (user_gpio == GPIO_PINMAP[EVT_AND] || user_gpio == GPIO_PINMAP[EVT_XOR]){
        
        if (user_gpio == GPIO_PINMAP[pigpioHandler->samplingTriggerSignal]){
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

        if (user_gpio == GPIO_PINMAP[TIMEPULSE]) {
//			std::cout<<"Timepulse"<<std::endl;
            struct timespec ts;
			clock_gettime(CLOCK_REALTIME, &ts);
            qint32 t_diff_us=ts.tv_nsec/1000;
            if (t_diff_us>500000L) t_diff_us=t_diff_us-1000000L;
            emit pigpioHandler->timePulseDiff(t_diff_us);
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

PigpiodHandler::PigpiodHandler(QVector<unsigned int> gpio_pins, unsigned int spi_freq, uint32_t spi_flags, QObject *parent)
	: QObject(parent)
{
    startOfProgram = QDateTime::currentDateTimeUtc();
    lastSamplingTime = startOfProgram;
    elapsedEventTimer.start();
    pigHandlerAddress = this;
    spiClkFreq = spi_freq;
    spiFlags = spi_flags;
    pi = pigpio_start((char*)"127.0.0.1", (char*)"8888");
    if (pi < 0) {
        qDebug() << "could not start pigpio. Is pigpiod running?";
        qDebug() << "you can start pigpiod with: sudo pigpiod -s 1";
        return;
    }
    for (auto& gpio_pin : gpio_pins) {
        set_mode(pi, gpio_pin, PI_INPUT);
        if (gpio_pin==GPIO_PINMAP[ADC_READY]) set_pull_up_down(pi, gpio_pin, PI_PUD_UP);
        if (gpio_pin!=20){
            callback(pi, gpio_pin, RISING_EDGE, cbFunction);
        }else{
            //qDebug() << "set callback for pin " << gpio_pin;
            callback(pi, gpio_pin, FALLING_EDGE, cbFunction);
        }
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

void PigpiodHandler::writeSpi(uint8_t command, std::string data){
    if(!spiInitialised){
        if(!spiInitialise()){
            return;
        }
    }
    char txBuf[data.size()+1];
    txBuf[0] = (char)command;
    for (int i = 1; i < data.size() +1; i++){
        txBuf[i] = data[i-1];
    }
    /*qDebug() << "trying to write: ";
    for (int i = 0; i < data.size()+1; i++){
        qDebug() << hex << (uint8_t)txBuf[i];
    }*/
    char rxBuf[data.size()+1];
    if (spi_xfer(pi, spiHandle, txBuf, rxBuf, data.size()+1)!=1+data.size()){
        qDebug() << "wrong number of bytes transfered";
        return;
    }
}

void PigpiodHandler::readSpi(uint8_t command, unsigned int bytesToRead){
    if(!spiInitialised){
        if(!spiInitialise()){
            return;
        }
    }
    //char buf[bytesToRead];
    //char charCommand = command;
    //spi_write(pi, spiHandle, &charCommand, 1);
    //spi_read(pi, spiHandle, buf, bytesToRead);

    char rxBuf[bytesToRead+1];
    char txBuf[bytesToRead+1];
    txBuf[0] = (char)command;
    for (int i = 1; i < bytesToRead; i++){
        txBuf[i] = 0;
    }
    if (spi_xfer(pi, spiHandle, txBuf, rxBuf, bytesToRead+1)!=1+bytesToRead){
        qDebug() << "wrong number of bytes transfered";
        return;
    }

    std::string data;
    for (int i = 1; i < bytesToRead+1; i++){
        data += rxBuf[i];
    }
    /*qDebug() << "read back: ";
    for (int i = 0; i < data.size(); i++){
        qDebug() << hex << (uint8_t)data[i];
    }
    qDebug() << ".";
    */
    emit spiData(command, data);
}

bool PigpiodHandler::initialised(){
    return isInitialised;
}

bool PigpiodHandler::isSpiInitialised(){
    return spiInitialised;
}

bool PigpiodHandler::spiInitialise(){
    if (!isInitialised){
        qDebug() << "pigpiohandler not initialised";
        return false;
    }
    if (spiInitialised){
        return true;
    }
    spiHandle = spi_open(pi, 0, spiClkFreq, spiFlags);
    if (spiHandle<0){
        qDebug() << "could not initialise spi bus";
        switch (spiHandle){
        case PI_BAD_CHANNEL:
            qDebug() << "DMA channel not 0-15";
            break;
        case PI_BAD_SPI_SPEED:
            qDebug() << "bad SPI speed";
            break;
        case PI_BAD_FLAGS:
            qDebug() << "bad spi open flags";
            break;
        case PI_NO_AUX_SPI:
            qDebug() << "no auxiliary SPI on Pi A or B";
            break;
        case PI_SPI_OPEN_FAILED:
            qDebug() << "can't open SPI device";
            break;
        default:
            break;
        }
        return false;
    }
    spiInitialised = true;
    return true;
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
