#ifndef _ADS1115_H_
#define _ADS1115_H_

#include <thread>
#include <chrono>
//#include <queue>
//#include <list>
#include <mutex>
#include <functional>

#include "hardware/i2c/i2cdevice.h"
#include "hardware/device_types.h"

// ADC ADS1x13/4/5 sampling readout delay
constexpr unsigned int READ_WAIT_DELAY_US_INIT { 10 };

/* ADS1115: 4(2) ch, 16 bit ADC  */

class ADS1115 : public i2cDevice, public DeviceFunction<DeviceType::ADC>, public static_device_base<ADS1115> {
public:
// 	struct Sample {
// 		std::chrono::time_point<std::chrono::steady_clock> timestamp;
// 		int16_t value;
// 		float voltage;
// 		float lsb_voltage;
// 		uint8_t channel;
// 		bool operator==(const Sample& other);
// 		bool operator!=(const Sample& other);
// 	};
// 	static constexpr Sample InvalidSample { std::chrono::steady_clock::time_point::min(), 0, 0., 0., 0 };

	typedef std::function<void(Sample)> SampleCallbackType;
	
	static constexpr int16_t MIN_ADC_VALUE { -32768 };
	static constexpr int16_t MAX_ADC_VALUE { 32767 };
	static constexpr uint16_t FULL_SCALE_RANGE { 65535 };

	enum CFG_CHANNEL { CH0 = 0, CH1, CH2, CH3 };
	enum CFG_DIFF_CHANNEL { CH0_1 = 0, CH0_3, CH1_3, CH2_3 };
	enum CFG_PGA { PGA6V = 0, PGA4V = 1, PGA2V = 2, PGA1V = 3, PGA512MV = 4, PGA256MV = 5 };
	enum CFG_RATES { SPS8 = 0x00, SPS16 = 0x01, SPS32 = 0x02, SPS64 = 0x03, SPS128 = 0x04, SPS250 = 0x05, SPS475 = 0x06, SPS860 = 0x07 };
	enum class CONV_MODE { UNKNOWN, SINGLE, CONTINUOUS };

	static auto adcToVoltage( int16_t adc, const CFG_PGA pga_setting ) -> float;

	ADS1115();
    ADS1115(uint8_t slaveAddress);
    ADS1115(const char* busAddress, uint8_t slaveAddress);
    ADS1115(const char* busAddress, uint8_t slaveAddress, CFG_PGA pga);
	virtual ~ADS1115();
	
    bool identify() override;
	
	void setActiveChannel( uint8_t channel, bool differential_mode = false );
    void setPga(CFG_PGA pga) { fPga[0] = fPga[1] = fPga[2] = fPga[3] = pga; }
    void setPga(unsigned int pga) { setPga((CFG_PGA)pga); }
    void setPga(uint8_t channel, CFG_PGA pga);
    void setPga(uint8_t channel, uint8_t pga) { setPga(channel, (CFG_PGA)pga); }
    CFG_PGA getPga(int ch) const { return fPga[ch]; }
    void setAGC(bool state);
    void setAGC(uint8_t channel, bool state);
    bool getAGC(uint8_t channel) const;
    void setRate(unsigned int rate) { fRate = rate & 0x07; }
    unsigned int getRate() const { return fRate; }
    bool setLowThreshold(int16_t thr);
    bool setHighThreshold(int16_t thr);
    int16_t readADC(unsigned int channel);
    double getVoltage(unsigned int channel) override;
    void getVoltage(unsigned int channel, double& voltage);
    void getVoltage(unsigned int channel, int16_t& adc, double& voltage);
    bool devicePresent();
    void setDiffMode(bool mode) { fDiffMode = mode; }
    bool setDataReadyPinMode();
    unsigned int getReadWaitDelay() const { return fReadWaitDelay; }
//     double getLastConvTime() const { return fLastConvTime; }
    bool setContinuousSampling( bool cont_sampling = true );
 	bool triggerConversion( unsigned int channel ) override;
//     void registerConversionReadyCallback(std::function<void(Sample)> fn) {	fConvReadyFn = fn; }
	Sample getSample( unsigned int channel ) override;
	Sample conversionFinished();
    
protected:
    CFG_PGA fPga[4] { PGA4V, PGA4V, PGA4V, PGA4V };
    uint8_t fRate { 0x00 };
	uint8_t fCurrentChannel { 0 };
	uint8_t fSelectedChannel { 0 };
//     double fLastConvTime { 0. };
    unsigned int fReadWaitDelay { READ_WAIT_DELAY_US_INIT }; ///< conversion wait time in us
    bool fAGC[4] { false, false, false, false }; ///< software agc which switches over to a better pga setting if voltage too low/high
    bool fDiffMode { false }; ///< measure differential input signals (true) or single ended (false=default)
	CONV_MODE fConvMode { CONV_MODE::UNKNOWN };
	Sample fLastSample[4] { InvalidSample, InvalidSample, InvalidSample, InvalidSample };
	
	std::mutex fMutex;
// 	std::function<void(Sample)> fConvReadyFn { };
	bool fTriggered { false };
	
    enum REG : uint8_t {
		CONVERSION = 0x00,
		CONFIG = 0x01,
		LO_THRESH = 0x02,
		HI_THRESH = 0x03
	};
    
    virtual void init();
    bool writeConfig( bool startNewConversion = false );
	bool setCompQueue( uint8_t bitpattern );
	bool readConversionResult( int16_t& dataword );
	static constexpr auto lsb_voltage( const CFG_PGA pga_setting ) -> float { return ( PGAGAINS[pga_setting]/MAX_ADC_VALUE ); }
	void waitConversionFinished(bool& error);
private:
    static constexpr float PGAGAINS[6] { 6.144, 4.096, 2.048, 1.024, 0.512, 0.256 };
};

#endif // !_ADS1115_H_
