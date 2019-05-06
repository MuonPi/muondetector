#ifndef _BME280_H_
#define _BME280_H_

#include "../i2cdevice.h"

/* BME280  */

class BME280 : public i2cDevice { // t_max = 112.8 ms for all three measurements at max oversampling
public:
	BME280() : i2cDevice(0x76) { init(); }
	BME280(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { init(); }
	BME280(uint8_t slaveAddress) : i2cDevice(slaveAddress) { init(); }

	bool init();
	unsigned int status();
	void measure();
	unsigned int readConfig();
	unsigned int read_CtrlMeasReg();
	bool writeConfig(uint8_t config);
	bool write_CtrlMeasReg(uint8_t config);
	bool setMode(uint8_t mode); // 3 bits: "1=sleep", "2=single shot", "3=interval"
	bool setTSamplingMode(uint8_t mode);
	bool setPSamplingMode(uint8_t mode);
	bool setHSamplingMode(uint8_t mode);
	bool softReset();
	void readCalibParameters();
	inline bool isCalibValid() const { return fCalibrationValid; }
	signed int getCalibParameter(unsigned int param) const;
	unsigned int readUT();
	unsigned int readUP();
	unsigned int readUH();
	TPH readTPCU();
	TPH getTPHValues();
	double getTemperature(signed int adc_T);
	double getPressure(signed int adc_P);
	double getPressure(signed int adc_P, signed int t_fine);
	double getHumidity(signed int adc_H);
	double getHumidity(signed int adc_H, signed int t_fine);
private:
	int32_t fT_fine = 0;
	unsigned int fLastConvTime;
	bool fCalibrationValid;
	uint16_t fCalibParameters[18];	// 18x 16-Bit words in 36 8-Bit registers
};

#endif // !_BME280_H_