#include "i2chandler.h"
#include "i2c/i2cdevices.h"
#include "tcpmessage.h"
#include "tcpmessage_keys.h"
#include "config.h"
#include "muondetector_structs.h"
#include "logparameter.h"
#include "helper_classes/property.h"
#include "gpio_pin_definitions.h"

// for i2cdetect:
extern "C" {
#include "i2c/custom_i2cdetect.h"
}

#include <string>
#include <iomanip>
#include <QDebug>

static unsigned int HW_VERSION = 0; // default value is set in calibration.h

I2cHandler::I2cHandler(uint8_t _pcaPortMask, unsigned int _eventTrigger, float *_dacThresh, float _biasVoltage, bool *polarities,
                       bool *preamps, bool _biasOn, bool gain, int verbose, QObject *parent) : QObject(parent)
{
    // instantiate, detect and initialize all other i2c devices
    // LM75A temp sensor
    lm75 = new LM75();
    if (lm75->devicePresent()) {
        if (verbose>2) {
            qInfo()<<"LM75 device is present.";
            qDebug()<<"temperature is "<<lm75->getTemperature()<< DEGREE_CHARCODE <<"C";
            qDebug()<<"readout took"<<lm75->getLastTimeInterval()<<"ms";
        }
    } else {
        qWarning()<<"LM75 device NOT present!";
    }
    // 4ch, 16/14bit ADC ADS1115/1015
    adc = new ADS1115();
    if (adc->devicePresent()) {
        adc->setPga(ADS1115::PGA4V);
        //adc->setPga(0, ADS1115::PGA2V);
        //		adc->setRate(0x06);  // ADS1115::RATE475
        adc->setRate(0x07);  // ADS1115::RATE860
        adc->setAGC(false);
        if (!adc->setDataReadyPinMode()) {
            qWarning()<<"error: failed setting data ready pin mode (setting thresh regs)";
        }

        // set up sampling timer used for continuous sampling mode
        samplingTimer.setInterval(MuonPi::Config::Hardware::trace_sampling_interval);
        samplingTimer.setSingleShot(false);
        samplingTimer.setTimerType(Qt::PreciseTimer);
        connect(&samplingTimer, &QTimer::timeout, this, &I2cHandler::sampleAdc0TraceEvent);

        // set up peak sampling mode
        setAdcSamplingMode(static_cast<uint8_t>(ADC_SAMPLING_MODE::ADC_MODE_PEAK));

        if (verbose>2) {
            qInfo()<<"ADS1115 device is present.";
            bool ok=adc->setLowThreshold(0b00000000);
            ok = ok && adc->setHighThreshold(0b10000000);
            if (ok) qDebug()<<"successfully setting threshold registers";
            else qWarning()<<"error: failed setting threshold registers";
            qDebug()<<"single ended channels:";
            qDebug()<<"ch0: "<<adc->readADC(0)<<" ch1: "<<adc->readADC(1)
                   <<" ch2: "<<adc->readADC(2)<<" ch3: "<<adc->readADC(3);
            adc->setDiffMode(true);
            qDebug()<<"diff channels:";
            qDebug()<<"ch0-1: "<<adc->readADC(0)<<" ch0-3: "<<adc->readADC(1)
                   <<" ch1-3: "<<adc->readADC(2)<<" ch2-3: "<<adc->readADC(3);
            adc->setDiffMode(false);
            qDebug()<<"readout took "<<adc->getLastTimeInterval()<<" ms";
        }
    } else {
        adcSamplingMode =static_cast<uint8_t>(ADC_SAMPLING_MODE::ADC_MODE_DISABLED);
        qWarning()<<"ADS1115 device NOT present!";
    }

    // 4ch DAC MCP4728
    dac = new MCP4728();
    if (dac->devicePresent()) {
        if (verbose>2) {
            qInfo()<<"MCP4728 device is present.";
            qDebug()<<"DAC registers / output voltages:";
            for (int i=0; i<4; i++) {
                MCP4728::DacChannel dacChannel;
                MCP4728::DacChannel eepromChannel;
                eepromChannel.eeprom=true;
                dac->readChannel(i, dacChannel);
                dac->readChannel(i, eepromChannel);
                qDebug()<<"  ch"<<i<<": "<<dacChannel.value<<" = "<<MCP4728::code2voltage(dacChannel)<<" V"
                "  (stored: "<<eepromChannel.value<<" = "<<MCP4728::code2voltage(eepromChannel)<<" V)";
            }
            qDebug()<<"readout took "<<dac->getLastTimeInterval()<<" ms";
        }
    } else {
        qCritical("MCP4728 device NOT present!");
        // this error is critical, since the whole concept of counting muons is based on
        // the function of the threshold discriminator
        // we should quit here returning an error code (?)
        //exit(-1);
    }
    float *tempThresh = _dacThresh;
    for (int i=std::min(DAC_TH1, DAC_TH2); i<=std::max(DAC_TH1, DAC_TH2); i++) {
        if (tempThresh[i]<0. && dac->devicePresent()) {
            MCP4728::DacChannel dacChannel;
            MCP4728::DacChannel eepromChannel;
            eepromChannel.eeprom=true;
            dac->readChannel(i, dacChannel);
            dac->readChannel(i, eepromChannel);
            tempThresh[i]=MCP4728::code2voltage(dacChannel);
            //tempThresh[i]=code2voltage(eepromChannel);
        }
    }
    dacThresh.push_back(tempThresh[0]);
    dacThresh.push_back(tempThresh[1]);


    if (dac->devicePresent()) {
        MCP4728::DacChannel eeprom_value;
        eeprom_value.eeprom = true;
        dac->readChannel(DAC_BIAS, eeprom_value);
        if (eeprom_value.value == 0) {
            setBiasVoltage(MuonPi::Config::Hardware::DAC::Voltage::bias);
            setDacThresh(0, MuonPi::Config::Hardware::DAC::Voltage::threshold[0]);
            setDacThresh(1, MuonPi::Config::Hardware::DAC::Voltage::threshold[1]);
        }
    }

    biasVoltage = _biasVoltage;
    if (biasVoltage<0. && dac->devicePresent()) {
        MCP4728::DacChannel dacChannel;
        MCP4728::DacChannel eepromChannel;
        eepromChannel.eeprom=true;
        dac->readChannel(DAC_BIAS, dacChannel);
        dac->readChannel(DAC_BIAS, eepromChannel);
        biasVoltage=MCP4728::code2voltage(dacChannel);
        //tempThresh[i]=code2voltage(eepromChannel);
    }

    // PCA9536 4 bit I/O I2C device used for selecting the UBX timing input
    pca = new PCA9536();
    if (pca->devicePresent()) {
        if (verbose>2) {
            qInfo()<<"PCA9536 device is present."<<std::endl;
            qDebug()<<" inputs: 0x"<<std::hex<<(int)pca->getInputState();
            qDebug()<<"readout took "<<std::dec<<pca->getLastTimeInterval()<<" ms";
        }
        pca->setOutputPorts(0b00000111);
        setPcaChannel(_pcaPortMask);
    } else {
        qCritical()<<"PCA9536 device NOT present!";
    }

    if (dac->devicePresent()) {
        if (dacThresh[0] > 0) dac->setVoltage(DAC_TH1, dacThresh[0]);
        if (dacThresh[1] > 0) dac->setVoltage(DAC_TH2, dacThresh[1]);
        if (biasVoltage > 0) dac->setVoltage(DAC_BIAS, biasVoltage);
    }


    // removed initialization of ublox i2c interface since it doesn't work properly on RPi
    // the Ublox i2c relies on clock stretching, which RPi is not supporting
    // the ublox's i2c address is still occupied but locked, i.e. access is prohibited
    ubloxI2c = new UbloxI2c(0x42);
    ubloxI2c->lock();
    if (ubloxI2c->devicePresent()) {
        if (verbose>2) {
            qInfo()<<"ublox I2C device interface is present.";
            uint16_t bufcnt = 0;
            bool ok = ubloxI2c->getTxBufCount(bufcnt);
            if (ok) qDebug()<<"bytes in TX buf: "<<bufcnt;

            //unsigned long int argh=0;
            //while (argh++<100UL) {
            //bufcnt = 0;
            //ok = ubloxI2c->getTxBufCount(bufcnt);
            //if (ok) std::cout<<"bytes in TX buf: "<<hex<<bufcnt<<dec<<std::endl;
            //std::string str=ubloxI2c->getData();
            //std::cout<<"string length: "<<str.size()<<std::endl;
            //usleep(200000L);
            //}
            //ubloxI2c->getData();
            //ok = ubloxI2c->getTxBufCount(bufcnt);
            //if (ok) std::cout<<"bytes in TX buf: "<<bufcnt<<std::endl;

        }
    } else {
        //std::cout<<"ublox I2C device interface NOT present"<<std::endl;
    }

    // check if also an Adafruit-SSD1306 compatible i2c OLED display is present
    // initialize and start loop for display of several state variables
    oled = new Adafruit_SSD1306(0x3c);
    if (oled->devicePresent()) {
        if (verbose>-1) {
            qInfo()<<"I2C SSD1306-type OLED display found at address 0x3c";
        }
        oled->begin();
        oled->clearDisplay();

        // text display tests
        oled->setTextSize(1);
        oled->setTextColor(Adafruit_SSD1306::WHITE);
        oled->setCursor(0,2);
        oled->print("*Cosmic Shower Det.*\n");
        oled->print("V");
        oled->print(MuonPi::Version::software.string().c_str());
        oled->print("\n");
        //oled->print("V 1.2.0\n");
        //  display.setTextColor(BLACK, WHITE); // 'inverted' text
        /*
        struct timespec tNow;
        clock_gettime(CLOCK_REALTIME, &tNow);

        oled->printf("time:\n%ld.%06d\n", tNow.tv_sec, tNow.tv_nsec/1000L);
        oled->printf("  %02d s", tNow.tv_sec%60);
        oled->drawHorizontalBargraph(0,24,128,8,1, (tNow.tv_sec%60)*10/6);
*/
        //  display.setTextSize(1);
        //  display.setTextColor(WHITE);
        //  display.printf("0x%8X\n", 0xDEADBEEF);
        oled->display();
        usleep(500000L);
        connect(&oledUpdateTimer, SIGNAL(timeout()), this, SLOT(updateOledDisplay()));

        oledUpdateTimer.start(MuonPi::Config::Hardware::OLED::update_interval);
    } else {
        //std::cout<<"I2C OLED display NOT present"<<std::endl;
    }

    // for pigpio signals:
    preampStatus[0] = preamps[0];
    preampStatus[1] = preamps[1];
    gainSwitch= gain;
    biasON = _biasOn;
    eventTrigger = (GPIO_PIN)_eventTrigger;
    polarity1 = polarities[0];
    polarity2 = polarities[1];


    // for diagnostics:
    // print out some i2c device statistics
    if (verbose>3) {
        std::cout<<"Nr. of invoked I2C devices (plain count): "<<i2cDevice::getNrDevices()<<std::endl;
        std::cout<<"Nr. of invoked I2C devices (gl. device list's size): "<<i2cDevice::getGlobalDeviceList().size()<<std::endl;
        std::cout<<"Nr. of bytes read on I2C bus: "<<i2cDevice::getGlobalNrBytesRead()<<std::endl;
        std::cout<<"Nr. of bytes written on I2C bus: "<<i2cDevice::getGlobalNrBytesWritten()<<std::endl;
        std::cout<<"list of device addresses: "<<std::endl;
        for (uint8_t i=0; i<i2cDevice::getGlobalDeviceList().size(); i++)
        {
            std::cout<<(int)i+1<<" 0x"<<hex<<(int)i2cDevice::getGlobalDeviceList()[i]->getAddress()<<" "<<i2cDevice::getGlobalDeviceList()[i]->getTitle();
            if (i2cDevice::getGlobalDeviceList()[i]->devicePresent()) std::cout<<" present"<<std::endl;
            else std::cout<<" missing"<<std::endl;
        }
        lm75->getCapabilities();
    }
}

I2cHandler::~I2cHandler(){
    if (pca!=nullptr){ delete pca; pca = nullptr; }
    if (dac!=nullptr){ delete dac; dac = nullptr; }
    if (adc!=nullptr){ delete adc; adc = nullptr; }
    if (eep!=nullptr){ delete eep; eep = nullptr; }
    if (ubloxI2c!=nullptr){ delete ubloxI2c; ubloxI2c = nullptr; }
    if (oled!=nullptr){ delete oled; oled = nullptr; }
}

void I2cHandler::setAdcSamplingMode(uint8_t mode) {
    if (mode > static_cast<uint8_t>(ADC_SAMPLING_MODE::ADC_MODE_TRACE)) return;
    adcSamplingMode=mode;
    if (mode == static_cast<uint8_t>(ADC_SAMPLING_MODE::ADC_MODE_TRACE)) samplingTimer.start();
    else samplingTimer.stop();
}

void I2cHandler::sampleAdc0Event(){
    const uint8_t channel=0;
    if (adc==nullptr || adcSamplingMode == static_cast<uint8_t>(ADC_SAMPLING_MODE::ADC_MODE_DISABLED)){
        return;
    }
    if (adc->getStatus() & i2cDevice::MODE_UNREACHABLE) return;

    // send message to gui containing value
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_ADC_SAMPLE);
    float value = adc->readVoltage(channel);
    *(tcpMessage.dStream) << (quint8)channel << value;
    emit sendTcpMessage(tcpMessage);

    // send message with value to be added to correct histogram
    QString name="pulseHeight";
    // histoMap[name].fill(value); // replaced
    emit fillHisto(name, value);

    // send log parameter
    emit logParameter(LogParameter("adcSamplingTime", QString::number(adc->getLastConvTime())+" ms", LogParameter::LOG_AVERAGE));
    name="adcSampleTime";

    // checkRescaleHisto(histoMap[name], adc->getLastConvTime()); // replaced
    // histoMap[name].fill(adc->getLastConvTime()); // replaced
    emit fillHisto(name, adc->getLastConvTime());
    currentAdcSampleIndex=0;
}

void I2cHandler::sampleAdc0TraceEvent(){
    const uint8_t channel=0;
    if (adc==nullptr || adcSamplingMode == static_cast<uint8_t>(ADC_SAMPLING_MODE::ADC_MODE_DISABLED)){
        return;
    }
    if (adc->getStatus() & i2cDevice::MODE_UNREACHABLE) return;
    float value = adc->readVoltage(channel);
    adcSamplesBuffer.push_back(value);
    if (adcSamplesBuffer.size() > MuonPi::Config::Hardware::ADC::buffer_size/*ADC_SAMPLEBUFFER_SIZE*/) adcSamplesBuffer.pop_front();
    if (currentAdcSampleIndex>=0) {
        currentAdcSampleIndex++;
        if (currentAdcSampleIndex >= (MuonPi::Config::Hardware::ADC::buffer_size - MuonPi::Config::Hardware::ADC::pretrigger)/*ADC_SAMPLEBUFFER_SIZE-ADC_PRETRIGGER*/) {
            TcpMessage tcpMessage(TCP_MSG_KEY::MSG_ADC_TRACE);
            *(tcpMessage.dStream) << (quint16) adcSamplesBuffer.size();
            for (int i=0; i<adcSamplesBuffer.size(); i++)
                *(tcpMessage.dStream) << adcSamplesBuffer[i];
            emit sendTcpMessage(tcpMessage);
            currentAdcSampleIndex = -1;
            //qDebug()<<"adc trace sent";
        }
    }
    emit logParameter(LogParameter("adcSamplingTime", QString::number(adc->getLastConvTime())+" ms", LogParameter::LOG_AVERAGE));
    //checkRescaleHisto(histoMap["adcSampleTime"], adc->getLastConvTime());
    // histoMap["adcSampleTime"].fill(adc->getLastConvTime()); // replaced
    emit fillHisto("adcSampleTime",adc->getLastConvTime());
}

void I2cHandler::sampleAdcEvent(uint8_t channel){
    if (adc==nullptr || adcSamplingMode == static_cast<uint8_t>(ADC_SAMPLING_MODE::ADC_MODE_DISABLED)){
        return;
    }
    if (adc->getStatus() & i2cDevice::MODE_UNREACHABLE) return;
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_ADC_SAMPLE);
    float value = adc->readVoltage(channel);
    *(tcpMessage.dStream) << (quint8)channel << value;
    emit sendTcpMessage(tcpMessage);
}

void I2cHandler::getTemperature(){
    if (lm75==nullptr){
        return;
    }
    TcpMessage tcpMessage(TCP_MSG_KEY::MSG_TEMPERATURE);
    if (lm75->getStatus() & i2cDevice::MODE_UNREACHABLE) return;
    float value = lm75->getTemperature();
    *(tcpMessage.dStream) << value;
    emit sendTcpMessage(tcpMessage);
}

// ALL FUNCTIONS ABOUT SETTINGS FOR THE I2C-DEVICES (DAC, ADC, PCA...)
void I2cHandler::setPcaChannel(uint8_t channel) {
    if (!pca || !pca->devicePresent()) {
        return;
    }
    if (channel > ((HW_VERSION==1)?3:7)) {
        qWarning() << "invalid PCA channel selection: ch" <<(int)channel<<"...ignoring";
        return;
    }
    if (verbose > 0){
        qInfo() << "changed pcaPortMask to " << channel;
    }
    pcaPortMask = channel;
    pca->setOutputState(channel);
    emit logParameter(LogParameter("ubxInputSwitch", "0x"+QString::number(pcaPortMask,16), LogParameter::LOG_EVERY));
    //sendPcaChannel();
}

void I2cHandler::setBiasVoltage(float voltage) {
    biasVoltage = voltage;
    if (verbose > 0){
        qInfo() << "change biasVoltage to " << voltage;
    }
    if (dac && dac->devicePresent()) {
        dac->setVoltage(DAC_BIAS, voltage);
        emit logParameter(LogParameter("biasDAC", QString::number(voltage)+" V", LogParameter::LOG_EVERY));
    }
    emit clearRates();
    //sendBiasVoltage();
}

void I2cHandler::setDacThresh(uint8_t channel, float threshold) {
    if (threshold < 0 || channel > 3) { return; }
    if (channel==2 || channel==3) {
        if (dac->devicePresent()) {
            dac->setVoltage(channel, threshold);
        }
        return;
    }
    if (threshold > 4.095){
        threshold = 4.095;
    }
    if (verbose > 0){
        qInfo() << "change dacThresh " << channel << " to " << threshold;
    }
    dacThresh[channel] = threshold;
    emit clearRates();
    if (dac->devicePresent()) {
        dac->setVoltage(channel, threshold);
        emit logParameter(LogParameter("thresh"+QString::number(channel+1), QString::number(dacThresh[channel])+" V", LogParameter::LOG_EVERY));
    }
    //sendDacThresh(channel);
}

void I2cHandler::saveDacValuesToEeprom(){
    for (int i=0; i<4; i++) {
        MCP4728::DacChannel dacChannel;
        dac->readChannel(i, dacChannel);
        dacChannel.eeprom=true;
        dac->writeChannel(i, dacChannel);
    }
}

bool I2cHandler::readEeprom()
{
    if (eep==nullptr) return false;
    if (eep->devicePresent()) {
        if (verbose>2) std::cout<<"eep device is present."<<std::endl;
    } else {
        std::cerr<<"eeprom device NOT present!"<<std::endl;
        return false;
    }
    uint16_t n=256;
    uint8_t buf[256];
    for (int i=0; i<n; i++) buf[i]=0;
    bool retval=(eep->readBytes(0,n,buf)==n);
    std::cout<<"*** EEPROM content ***"<<std::endl;
    for (int j=0; j<16; j++) {
        std::cout<<std::hex<<std::setfill ('0') << std::setw (2)<<j*16<<": ";
        for (int i=0; i<16; i++) {
            std::cout<<std::hex<<std::setfill ('0') << std::setw (2)<<(int)buf[j*16+i]<<" ";
        }
        std::cout<<std::endl;
    }
    return retval;
}

void I2cHandler::updateOledDisplay(Property &nrVisibleSats, Property &nrSats, Property &fixStatus, double and_rate, double xor_rate) {
    if (!oled->devicePresent()) return;
    oled->clearDisplay();
    oled->setCursor(0,2);
    oled->print("*Cosmic Shower Det.*\n");
//	oled->printf("rate (XOR) %4.2f 1/s\n", getRateFromCounts(XOR_RATE));
//	oled->printf("rate (AND) %4.2f 1/s\n", getRateFromCounts(AND_RATE));
    oled->printf("Rates %4.1f %4.1f /s\n", and_rate, xor_rate);
//	oled->printf("temp %4.2f %cC\n", lm75->lastTemperatureValue(), DEGREE_CHARCODE);
    oled->printf("temp %4.2f %cC\n", lm75->(), DEGREE_CHARCODE);
    oled->printf("%d(%d) Sats ", nrVisibleSats().toInt(), nrSats().toInt(), DEGREE_CHARCODE);
    oled->printf("%s\n", FIX_TYPE_STRINGS[fixStatus().toInt()].toStdString().c_str());
    oled->display();
}
