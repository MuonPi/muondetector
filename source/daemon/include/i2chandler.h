#ifndef I2CHANDLER_H
#define I2CHANDLER_H

#include "i2c/i2cdevices.h"
#include "logparameter.h"
#include "helper_classes/property.h"
#include <tcpmessage.h>
#include <QObject>
#include <QTimer>
#include <QList>
#include <vector>

#define DAC_BIAS 2 // channel of the dac where bias voltage is set
#define DAC_TH1 0 // channel of the dac where threshold 1 is set
#define DAC_TH2 1 // channel of the dac where threshold 2 is set
#define DEGREE_CHARCODE 248

enum class ADC_SAMPLING_MODE : uint8_t {ADC_MODE_DISABLED=0, ADC_MODE_PEAK=1, ADC_MODE_TRACE=2 };

enum GPIO_PIN;

struct RateScanInfo {
    uint8_t origPcaMask=0;
    GPIO_PIN origEventTrigger=GPIO_PIN::UNDEFINED_PIN;
    uint16_t lastEvtCounter=0;
    uint8_t thrChannel=0;
    float origThr=3.3;
    float thrIncrement=0.1;
    float minThr=0.05;
    float maxThr=3.3;
    float currentThr=0.;
    uint16_t nrLoops=0;
    uint16_t currentLoop=0;
};

struct RateScan {
//	void addScanPoint(double scanpar, double a_rate) { scanMap[scanpar].append(a_rate); }
    uint8_t origPcaMask=0;
    GPIO_PIN origEventTrigger=GPIO_PIN::UNDEFINED_PIN;
    float origScanPar=3.3;
    double minScanPar=0.;
    double maxScanPar=1.;
    double currentScanPar=0.;
    double scanParIncrement=0.;
    uint32_t currentCounts=0;
    double currentTimeInterval=0.;
    double maxTimeInterval=1.;
    uint16_t nrLoops=0;
    uint16_t currentLoop=0;
    QMap<double, double> scanMap;
};

class I2cHandler : public QObject
{
    Q_OBJECT
public:
    explicit I2cHandler(uint8_t _pcaPortMask, unsigned int _eventTrigger, float *_dacThresh, float _biasVoltage, bool *polarities,
                        bool *preamps, bool _biasOn = false, bool gain = false, int verbose = 0,  QObject* = nullptr);
    ~I2cHandler();
    void setAdcSamplingMode(uint8_t mode);

    void saveDacValuesToEeprom();
    void setBiasVoltage(float voltage);
    void setDacThresh(uint8_t channel, float threshold); // channel 0 or 1 ; threshold in volts
    void setPcaChannel(uint8_t channel); // channel 0 to 3 ( 0: coincidence ; 1: xor ; 2: discr 1 ; 3: discr 2 )
    bool readEeprom();
    void updateOledDisplay(Property &nrVisibleSats, Property &nrSats, Property &fixStatus, double and_rate, double xor_rate);
    void startRateScan(uint8_t channel);
    void doRateScanIteration(RateScanInfo* info, Property &events);
    void sampleAdc0Event();
    void sampleAdc0TraceEvent();
    void sampleAdcEvent(uint8_t channel);
    void getTemperature();
    void scanI2cBus();

    // getters
    const uint8_t getAdcSamplingMode() { return adcSamplingMode; }

private:
    // i2c devices
    MCP4728* dac = nullptr;
    ADS1115* adc = nullptr;
    PCA9536* pca = nullptr;
    LM75* lm75 = nullptr;
    EEPROM24AA02* eep = nullptr;
    UbloxI2c* ubloxI2c = nullptr;
    Adafruit_SSD1306* oled = nullptr;

    // devices related
    uint8_t adcSamplingMode = ADC_MODE_PEAK;
    QList<float> adcSamplesBuffer;
    uint16_t currentAdcSampleIndex = -1;
    uint8_t pcaPortMask = 0;
    vector<float> dacThresh; // do not give values here because of push_back in constructor
    float biasVoltage = 0.;
    bool biasON = false;
    bool gainSwitch = false;
    GPIO_PIN eventTrigger;
    bool preampStatus[2];
    bool polarity1 = true;	// input polarity switch: true=pos, false=neg
    bool polarity2 = true;

    // other stuff
    QTimer samplingTimer;
    QTimer oledUpdateTimer;
    int verbose;

signals:
    void setInhibited(bool);
    void logParameter(const LogParameter& log);
    void clearRates();
    void sendTcpMessage(TcpMessage tcpMessage);
    void fillHisto(QString histoName, float value);
};

#endif // I2CHANDLER_H
