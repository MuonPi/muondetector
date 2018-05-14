#include <stdio.h>
#include <fcntl.h>     // open
#include <unistd.h>
#include <sys/ioctl.h> // ioctl
#include <inttypes.h>  	    // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions for linux like systems
#include <sys/time.h>                // for gettimeofday()

#ifndef _I2CDEVICES_H_
#define _I2CDEVICES_H_

#define READ_WAIT_DELAY_INIT 10

//We define a class named i2cDevices to outsource the hardware dependant programm parts. We want to 
//access components of integrated curcuits, like the ads1115 or other subdevices via i2c-bus.
//The main aim here was, that the user does not have  to be concerned about the c like low level operations
//of the coding.
class i2cDevice {         				

public:							
	i2cDevice();
	i2cDevice(const char* busAddress);
	i2cDevice(uint8_t slaveAddress);
	i2cDevice(const char* busAddress, uint8_t slaveAddress);
	~i2cDevice();
						      
	void setAddress(uint8_t address);
	inline uint8_t getAddress() const { return fAddress;}				
	inline unsigned int getNrDevices() { return fNrDevices; }
	inline unsigned int getNrBytesRead() { return fNrBytesRead; }
	inline unsigned int getNrBytesWritten() { return fNrBytesWritten; }
	inline unsigned int getGlobalNrBytesRead() { return fGlobalNrBytesRead; }
	inline unsigned int getGlobalNrBytesWritten() { return fGlobalNrBytesWritten; }
	
	inline double getLastTimeInterval() { return fLastTimeInterval; }
	
	inline void setDebugLevel(int level) { fDebugLevel = level; }
	inline int getDebugLevel() { return fDebugLevel; }
	
	// read nBytes bytes into buffer buf
	// return value:
	// 	the number of bytes actually read if successful
	//	-1 on error
	int read(uint8_t* buf, int nBytes);

	// write nBytes bytes from buffer buf
	// return value:
	// 	the number of bytes actually written if successful
	//	-1 on error
	int write(uint8_t* buf, int nBytes);
	
	// write nBytes bytes from buffer buf in register reg
	// return value:
	// 	the number of bytes actually written if successful
	//	-1 on error
	// note: first byte of the write sequence is the register address,
	// the following bytes from buf are then written in a sequence
	int writeReg(uint8_t reg, uint8_t* buf, int nBytes);

	// read nBytes bytes into buffer buf from register reg
	// return value:
	// 	the number of bytes actually read if successful
	//	-1 on error
	// note: first writes reg address and after a repeated start 
	// reads in a sequence of bytes
	// not all devices support this procedure
	// refer to the device's datasheet
	int readReg(uint8_t reg, uint8_t* buf, int nBytes);
	
	int8_t readBit(uint8_t regAddr, uint8_t bitNum, uint8_t *data);
	int8_t readBits(uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t *data);
	int8_t readByte(uint8_t regAddr, uint8_t *data);
	int8_t readBytes(uint8_t regAddr, uint8_t length, uint8_t *data);
	bool writeBit(uint8_t regAddr, uint8_t bitNum, uint8_t data);
	bool writeBits(uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data);
	bool writeByte(uint8_t regAddr, uint8_t data);
	bool writeBytes(uint8_t regAddr, uint8_t length, uint8_t* data);
	bool writeWords(uint8_t regAddr, uint8_t length, uint16_t* data);
	bool writeWord(uint8_t regAddr, uint16_t data);


protected:
	int fHandle;
	uint8_t fAddress;
	static unsigned int fNrDevices;
	unsigned long int fNrBytesWritten;
	unsigned long int fNrBytesRead;
	static unsigned long int fGlobalNrBytesRead;
	static unsigned long int fGlobalNrBytesWritten;
	double fLastTimeInterval;
	struct timeval fT1, fT2;
	int fDebugLevel;
	
	// fuctions for measuring time intervals
	void startTimer();
	void stopTimer();
};



class ADS1115 : public i2cDevice {
public:
	enum CFG_CHANNEL {CH0=0, CH1, CH2, CH3};
	enum CFG_RATE {RATE8=0, RATE16, RATE32, RATE64, RATE128, RATE250, RATE475, RATE860};
	enum CFG_PGA {PGA6V=0, PGA4V=1, PGA2V=2, PGA1V=3, PGA512MV=4, PGA256MV=5};
	static const double PGAGAINS[6];

	ADS1115() : i2cDevice("/dev/i2c-1",0x48) { init(); }
	ADS1115(uint8_t slaveAddress) : i2cDevice(slaveAddress) { init(); }
	ADS1115(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress,slaveAddress) { init(); }
	ADS1115(const char* busAddress, uint8_t slaveAddress, CFG_PGA pga) : i2cDevice(busAddress,slaveAddress)
	{
	  init();
	  setPga(pga);
	}

	inline void setPga(CFG_PGA pga) { fPga[0]=fPga[1]=fPga[2]=fPga[3]=pga; }
	inline void setPga(unsigned int pga) { setPga((CFG_PGA)pga); }
	inline CFG_PGA getPga(int ch) const { return fPga[ch]; }
	inline void setAGC(bool state) { fAGC=state; }
	inline bool getAGC() const { return fAGC; }
	inline void setRate(CFG_RATE rate) { fRate=rate; }
	inline CFG_RATE getRate() const { return fRate; }
	int16_t readADC(unsigned int channel);
	double readVoltage(unsigned int channel);
	void readVoltage(unsigned int channel, double& voltage);
	void readVoltage(unsigned int channel, int16_t& adc, double& voltage);

	inline unsigned int getReadWaitDelay() const { return fReadWaitDelay; }
	
protected:
	CFG_PGA fPga[4];
	CFG_RATE fRate;
	unsigned int fLastConvTime;
	unsigned int fLastADCValue;
	double fLastVoltage;
	unsigned int fReadWaitDelay;	// conversion wait time in us
	bool fAGC;
	
	inline void init() {
	  fPga[0]=fPga[1]=fPga[2]=fPga[3]=PGA4V;
	  fReadWaitDelay=READ_WAIT_DELAY_INIT;
	  fRate=RATE8;
	  fAGC=false;
	}
};


class ADS1015 : public ADS1115 {
public:
	using ADS1115::ADS1115;
	enum CFG_RATE { RATE8 = 0, RATE16, RATE32, RATE64, RATE128, RATE250, RATE475, RATE860 };
};


class MCP4728 : public i2cDevice {
	// the DAC supports writing to input register but not sending latch bit to update the output register
	// here we will always send the "UDAC" (latch) bit because we don't neet this functionality
	// MCP4728 listens to I2C Generall Call Commands
	// reset, wake-up, software update, read address bits
	// reset is "0x00 0x06"
	// wake-up is "0x00 0x09"
public:
	//enum CFG_CHANNEL {CH_A=0, CH_B, CH_C, CH_D};
	//enum POWER_DOWN {NORM=0, LOAD1, LOAD2, LOAD3};  // at Power Down mode the output is loaded with 1k, 100k, 500k
													// to ground to power down the circuit
	enum CFG_GAIN {GAIN1 = 1, GAIN2 = 2};
	MCP4728() : i2cDevice(0x60) {}
	MCP4728(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) {}
	MCP4728(uint8_t slaveAddress) : i2cDevice(slaveAddress) {}
	bool setVoltage(CFG_CHANNEL channel, float voltage, CFG_GAIN gain = GAIN1);
	bool setValue(CFG_CHANNEL channel, unsigned int value, CFG_GAIN gain = GAIN1);
};


class PCA9536 : public i2cDevice {
	// the device supports reading the incoming logic levels of the pins if set to input in the configuration register (will probably not use this feature)
	// the device supports polarity inversion (by configuring the polarity inversino register) (will probably not use this feature)
public:
	enum CFG_REG {INPUT=0, OUTPUT, POLARITY_INVERSION, CONFIG};
	enum CFG_PORT{C0=0, C1=2, C3=4, C4=8};
	PCA9536() : i2cDevice(0x41) {}
	PCA9536(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) {}
	PCA9536(uint8_t slaveAddress) : i2cDevice(slaveAddress) {}
	void setOutputPorts(uint8_t portMask);
};


class LM75 : public i2cDevice {
public:

	LM75() : i2cDevice(0x4c) {}
	LM75(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress,slaveAddress) {}
	LM75(uint8_t slaveAddress) : i2cDevice(slaveAddress) {}
	signed int readRaw();
	double getTemperature();

private:
	unsigned int fLastConvTime;
	signed int fLastRawValue;
	double fLastTemp;
};



class X9119 : public i2cDevice {
public:

	X9119() : i2cDevice(0x28) {}
	X9119(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress,slaveAddress) {}
	X9119(uint8_t slaveAddress) : i2cDevice(slaveAddress) {}

	unsigned int readWiperReg();
	unsigned int readWiperReg2();
	unsigned int readWiperReg3();
	void writeWiperReg(unsigned int value);
	unsigned int readDataReg(uint8_t reg);
	void writeDataReg(uint8_t reg, unsigned int value);
private:
	unsigned int fWiperReg;
};



class EEPROM24AA02 : public i2cDevice {
public:
	EEPROM24AA02() : i2cDevice(0x50) {}
	EEPROM24AA02(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress,slaveAddress) {}
	EEPROM24AA02(uint8_t slaveAddress) : i2cDevice(slaveAddress) {}

	uint8_t readByte(uint8_t addr);
	void writeByte(uint8_t addr, uint8_t data);
private:
};



class BMP180 : public i2cDevice {
public:

	BMP180() : i2cDevice(0x77) {}
	BMP180(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress,slaveAddress) {}
	BMP180(uint8_t slaveAddress) : i2cDevice(slaveAddress) {}
	
	bool init();
	void readCalibParameters();
	inline bool isCalibValid() const { return fCalibrationValid; }
	signed int getCalibParameter(unsigned int param) const;
	unsigned int readUT();
	unsigned int readUP(uint8_t oss);
	double getTemperature();
	double getPressure(uint8_t oss);

private:
	unsigned int fLastConvTime;
	bool fCalibrationValid;
	signed int fCalibParameters[11];
	
};



class HMC5883 : public i2cDevice {
public:

	// Resolution for the 8 gain settings in mG/LSB
	static const double GAIN[8];
	HMC5883() : i2cDevice(0x1e) {}
	HMC5883(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress,slaveAddress) {}
	HMC5883(uint8_t slaveAddress) : i2cDevice(slaveAddress) {}
	
	bool init();
	// gain range 0..7
	void setGain(uint8_t gain);
	uint8_t readGain();
//	uint8_t readGain2();
	bool getXYZRawValues(int &x, int &y, int &z);
	bool getXYZMagneticFields(double &x, double &y, double &z);
	bool readRDYBit();
	bool readLockBit();
	bool calibrate(int &x, int &y, int &z);


private:
	unsigned int fLastConvTime;
	bool fCalibrationValid;
	unsigned int fGain;
	signed int fCalibParameters[11];
	
};

#endif // _I2CDEVICES_H_