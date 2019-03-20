#include <stdio.h>
#include <fcntl.h>     // open
#include <unistd.h>
#include <sys/ioctl.h> // ioctl
#include <inttypes.h>  	    // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions for linux like systems
#include <sys/time.h>                // for gettimeofday()
#include <vector>
#include <string>

#ifndef _I2CDEVICES_H_
#define _I2CDEVICES_H_

// ADC ADS1x13/4/5 sampling readout delay
#define READ_WAIT_DELAY_INIT 10

// OLED defines
#define OLED_I2C_RESET RPI_V2_GPIO_P1_22 /* GPIO 25 pin 12  */
// Oled supported display
#define	OLED_ADAFRUIT_SPI_128x32	0
#define	OLED_ADAFRUIT_SPI_128x64	1
#define	OLED_ADAFRUIT_I2C_128x32	2
#define	OLED_ADAFRUIT_I2C_128x64	3
#define	OLED_SEEED_I2C_128x64			4
#define	OLED_SEEED_I2C_96x96			5
#define OLED_LAST_OLED						6 /* always last type, used in code to end array */


// struct to store temperature, pressure and humidity data in different ways
struct TPH {
	uint32_t adc_T;
	uint32_t adc_P;
	uint32_t adc_H;
	double T, P, H;
};

//We define a class named i2cDevices to outsource the hardware dependant programm parts. We want to 
//access components of integrated curcuits, like the ads1115 or other subdevices via i2c-bus.
//The main aim here was, that the user does not have  to be concerned about the c like low level operations
//of the coding.
class i2cDevice {

public:

	enum MODE { MODE_NONE=0, MODE_NORMAL=0x01, MODE_FORCE=0x02,
				MODE_UNREACHABLE=0x04, MODE_FAILED=0x08 };

	i2cDevice();
	i2cDevice(const char* busAddress);
	i2cDevice(uint8_t slaveAddress);
	i2cDevice(const char* busAddress, uint8_t slaveAddress);
	virtual ~i2cDevice();

	void setAddress(uint8_t address);
	uint8_t getAddress() const { return fAddress; }
	static unsigned int getNrDevices() { return fNrDevices; }
	unsigned int getNrBytesRead() { return fNrBytesRead; }
	unsigned int getNrBytesWritten() { return fNrBytesWritten; }
	static unsigned int getGlobalNrBytesRead() { return fGlobalNrBytesRead; }
	static unsigned int getGlobalNrBytesWritten() { return fGlobalNrBytesWritten; }
	static std::vector<i2cDevice*>& getGlobalDeviceList() { return fGlobalDeviceList; }
	virtual bool devicePresent();
	uint8_t getStatus() const { return fMode; }
	
	double getLastTimeInterval() { return fLastTimeInterval; }

	void setDebugLevel(int level) { fDebugLevel = level; }
	int getDebugLevel() { return fDebugLevel; }

	void setTitle(const std::string& a_title) { fTitle = a_title; }
	const std::string& getTitle() const { return fTitle; }

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
	bool readByte(uint8_t regAddr, uint8_t *data);
	int16_t readBytes(uint8_t regAddr, uint16_t length, uint8_t *data);
	bool writeBit(uint8_t regAddr, uint8_t bitNum, uint8_t data);
	bool writeBits(uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data);
	bool writeByte(uint8_t regAddr, uint8_t data);
	bool writeBytes(uint8_t regAddr, uint16_t length, uint8_t* data);
	bool writeWords(uint8_t regAddr, uint16_t length, uint16_t* data);
	bool writeWord(uint8_t regAddr, uint16_t data);

	void getCapabilities();


protected:
	int fHandle;
	uint8_t fAddress;
	static unsigned int fNrDevices;
	unsigned long int fNrBytesWritten;
	unsigned long int fNrBytesRead;
	static unsigned long int fGlobalNrBytesRead;
	static unsigned long int fGlobalNrBytesWritten;
	double fLastTimeInterval; // the last time measurement's result is stored here
	struct timeval fT1, fT2;
	int fDebugLevel;
	static std::vector<i2cDevice*> fGlobalDeviceList;
	std::string fTitle="I2C device";
	uint8_t fMode = MODE_NONE;

	// fuctions for measuring time intervals
	void startTimer();
	void stopTimer();
};


/* ADS1115: 4(2) ch, 16 bit ADC  */
class ADS1115 : public i2cDevice {
public:
	enum CFG_CHANNEL { CH0 = 0, CH1, CH2, CH3 };
	enum CFG_DIFF_CHANNEL { CH0_1 = 0, CH0_3, CH1_3, CH2_3 };
//	enum CFG_RATE { RATE8 = 0, RATE16, RATE32, RATE64, RATE128, RATE250, RATE475, RATE860 };
	enum CFG_PGA { PGA6V = 0, PGA4V = 1, PGA2V = 2, PGA1V = 3, PGA512MV = 4, PGA256MV = 5 };
	static const double PGAGAINS[6];

	ADS1115() : i2cDevice("/dev/i2c-1", 0x48) { init(); }
	ADS1115(uint8_t slaveAddress) : i2cDevice(slaveAddress) { init(); }
	ADS1115(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { init(); }
	ADS1115(const char* busAddress, uint8_t slaveAddress, CFG_PGA pga) : i2cDevice(busAddress, slaveAddress)
	{
		init();
		setPga(pga);
	}

	void setPga(CFG_PGA pga) { fPga[0] = fPga[1] = fPga[2] = fPga[3] = pga; }
	void setPga(unsigned int pga) { setPga((CFG_PGA)pga); }
	void setPga(uint8_t channel, CFG_PGA pga) { if (channel>3) return; fPga[channel] = pga; }
	void setPga(uint8_t channel, uint8_t pga) { setPga(channel, (CFG_PGA)pga); }
	CFG_PGA getPga(int ch) const { return fPga[ch]; }
	void setAGC(bool state) { fAGC = state; }
	bool getAGC() const { return fAGC; }
	void setRate(unsigned int rate) { fRate = rate & 0x07; }
	unsigned int getRate() const { return fRate; }
	bool setLowThreshold(int16_t thr);
	bool setHighThreshold(int16_t thr);
	int16_t readADC(unsigned int channel);
	double readVoltage(unsigned int channel);
	void readVoltage(unsigned int channel, double& voltage);
	void readVoltage(unsigned int channel, int16_t& adc, double& voltage);
	bool devicePresent();
	void setDiffMode(bool mode) { fDiffMode=mode; }
	bool setDataReadyPinMode();
	unsigned int getReadWaitDelay() const { return fReadWaitDelay; }
	double getLastConvTime() const { return fLastConvTime; }

protected:
	CFG_PGA fPga[4];
	unsigned int fRate;
	double fLastConvTime;
	unsigned int fLastADCValue;
	double fLastVoltage;
	unsigned int fReadWaitDelay;	// conversion wait time in us
	bool fAGC;	// software agc which switches over to a better pga setting if voltage too low/high
	bool fDiffMode=false;	// measure differential input signals (true) or single ended (false=default)
	
	inline virtual void init() {
		fPga[0] = fPga[1] = fPga[2] = fPga[3] = PGA4V;
		fReadWaitDelay = READ_WAIT_DELAY_INIT;
		fRate = 0x00;  // RATE8
		fAGC = false;
		fTitle = "ADS1115";
	}
};

/* ADS1015: 4(2) ch, 12 bit ADC  */
class ADS1015 : public ADS1115 {
public:
	using ADS1115::ADS1115;
//	enum CFG_RATE { RATE128 = 0, RATE250, RATE490, RATE920, RATE1600, RATE2400, RATE3300 };
	
protected:
	inline void init() {
		ADS1115::init();
		fTitle = "ADS1015";
	}
};


class MCP4728 : public i2cDevice {
	// the DAC supports writing to input register but not sending latch bit to update the output register
	// here we will always send the "UDAC" (latch) bit because we don't need this functionality
	// MCP4728 listens to I2C Generall Call Commands
	// reset, wake-up, software update, read address bits
	// reset is "0x00 0x06"
	// wake-up is "0x00 0x09"
public:
	//enum CFG_CHANNEL {CH_A=0, CH_B, CH_C, CH_D};
	//enum POWER_DOWN {NORM=0, LOAD1, LOAD2, LOAD3};  // at Power Down mode the output is loaded with 1k, 100k, 500k
													// to ground to power down the circuit
	const static float VDD;	// change, if device powered with different voltage
	enum CFG_GAIN { GAIN1 = 0, GAIN2 = 1 };
	enum CFG_VREF { VREF_VDD = 0, VREF_2V = 1 };

	// struct that characterizes one dac output channel
	// setting the eeprom flag enables access to the eeprom registers instead of the dac output registers
	struct DacChannel {
		uint8_t pd=0x00;
		CFG_GAIN gain=GAIN1;
		CFG_VREF vref=VREF_VDD;
		bool eeprom = false;
		uint16_t value = 0;
	};

	MCP4728() : i2cDevice(0x60) { fTitle = "MCP4728"; }
	MCP4728(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { fTitle = "MCP4728"; }
	MCP4728(uint8_t slaveAddress) : i2cDevice(slaveAddress) { fTitle = "MCP4728"; }
	bool devicePresent();
	bool setVoltage(uint8_t channel, float voltage, bool toEEPROM = false);
	bool setValue(uint8_t channel, uint16_t value, uint8_t gain = GAIN1, bool toEEPROM = false);
	bool setChannel(uint8_t channel, const DacChannel& channelData);
	bool readChannel(uint8_t channel, DacChannel& channelData);
	
	static float code2voltage(const DacChannel& channelData);
private:
	unsigned int fLastConvTime;
};


class PCA9536 : public i2cDevice {
	// the device supports reading the incoming logic levels of the pins if set to input in the configuration register (will probably not use this feature)
	// the device supports polarity inversion (by configuring the polarity inversino register) (will probably not use this feature)
public:
	enum CFG_REG { INPUT_REG = 0x00, OUTPUT_REG=0x01, POLARITY_INVERSION=0x02, CONFIG_REG=0x03 };
	enum CFG_PORT { C0 = 0, C1 = 2, C3 = 4, C4 = 8 };
	PCA9536() : i2cDevice(0x41) { fTitle = "PCA9536"; }
	PCA9536(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { fTitle = "PCA9536"; }
	PCA9536(uint8_t slaveAddress) : i2cDevice(slaveAddress) { fTitle = "PCA9536"; }
	bool setOutputPorts(uint8_t portMask);
	bool setOutputState(uint8_t portMask);
	uint8_t getInputState();
	bool devicePresent();
};


class LM75 : public i2cDevice {
public:
	LM75() : i2cDevice(0x4f) { fTitle = "LM75A"; }
	LM75(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { fTitle = "LM75A"; }
	LM75(uint8_t slaveAddress) : i2cDevice(slaveAddress) { fTitle = "LM75A"; }
	bool devicePresent();
	int16_t readRaw();
	double getTemperature();

private:
	unsigned int fLastConvTime;
	signed int fLastRawValue;
	double fLastTemp;
};



class X9119 : public i2cDevice {
public:

	X9119() : i2cDevice(0x28) { fTitle = "X9119"; }
	X9119(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { fTitle = "X9119"; }
	X9119(uint8_t slaveAddress) : i2cDevice(slaveAddress) { fTitle = "X9119"; }

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
	EEPROM24AA02() : i2cDevice(0x50) { fTitle = "24AA02"; }
	EEPROM24AA02(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { fTitle = "24AA02"; }
	EEPROM24AA02(uint8_t slaveAddress) : i2cDevice(slaveAddress) { fTitle = "24AA02"; }

	//bool devicePresent();
	uint8_t readByte(uint8_t addr);
	bool readByte(uint8_t addr, uint8_t* value);
	//the readBytes function is already defined in the base class
	//int8_t readBytes(uint8_t regAddr, uint16_t length, uint8_t *data);
	void writeByte(uint8_t addr, uint8_t data);
	bool writeBytes(uint8_t addr, uint16_t length, uint8_t* data);
private:
};


class SHT21 : public i2cDevice {
public:
	SHT21() : i2cDevice(0x40) { fTitle = "SHT21"; }
	SHT21(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { fTitle = "SHT21"; }
	SHT21(uint8_t slaveAddress) : i2cDevice(slaveAddress) { fTitle = "SHT21"; }

	uint16_t readUT();  //read temperature; nothing gets passed
	uint16_t readUH();   //read humidity;  nothing gets passed

	bool softReset();                 //reset, datasheet, page 9, Rückgabetyp in void geändert
	uint8_t readResolutionSettings();           
	void setResolutionSettings(uint8_t settingsByte);	//Sets the resolution Bits for humidity and temperature
	float getTemperature();   // calculates the temperature with the formula in the datasheet. Gets the solution of read_temp()
	float getHumidity();    // calculates the temperature with the formula in the datasheet. Gets the solution of read_hum();

private:
	bool checksumCorrect(uint8_t data[]); // expects 3 byte long data array; Source: https://www2.htw-dresden.de/~wiki_sn/index.php/SHT21
};


class BMP180 : public i2cDevice {
public:

	BMP180() : i2cDevice(0x77) { fTitle = "BMP180"; }
	BMP180(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { fTitle = "BMP180"; }
	BMP180(uint8_t slaveAddress) : i2cDevice(slaveAddress) { fTitle = "BMP180"; }

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


class HMC5883 : public i2cDevice {
public:

	// Resolution for the 8 gain settings in mG/LSB
	static const double GAIN[8];
	HMC5883() : i2cDevice(0x1e) { fTitle = "HMC5883"; }
	HMC5883(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { fTitle = "HMC5883"; }
	HMC5883(uint8_t slaveAddress) : i2cDevice(slaveAddress) { fTitle = "HMC5883"; }

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


/* Ublox GPS receiver, I2C interface */
class UbloxI2c : public i2cDevice {
public:
	UbloxI2c() : i2cDevice(0x42) { fTitle = "UBLOX I2C"; }
	UbloxI2c(const char* busAddress, uint8_t slaveAddress) : i2cDevice(busAddress, slaveAddress) { fTitle = "UBLOX I2C"; }
	UbloxI2c(uint8_t slaveAddress) : i2cDevice(slaveAddress) { fTitle = "UBLOX I2C"; }
	bool devicePresent();
	std::string getData();
	bool getTxBufCount(uint16_t& nrBytes);

private:
};


//class Adafruit_GFX;
#include "./Adafruit_GFX.h"

/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

These displays use SPI to communicate, 4 or 5 pins are required to  
interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution

02/18/2013 	Charles-Henri Hallard (http://hallard.me)
						Modified for compiling and use on Raspberry ArduiPi Board
						LCD size and connection are now passed as arguments on 
						the command line (no more #define on compilation needed)
						ArduiPi project documentation http://hallard.me/arduipi
 
*********************************************************************/
class Adafruit_SSD1306 : public i2cDevice, public Adafruit_GFX 
{
public:
  enum { BLACK=0, WHITE=1 };
  
  Adafruit_SSD1306()
  : i2cDevice(0x3c)
    { fTitle="SSD1306 OLED"; init(OLED_ADAFRUIT_I2C_128x64, -1); }
  Adafruit_SSD1306(const char* busAddress, uint8_t slaveAddress) 
    : i2cDevice(busAddress, slaveAddress)
    { fTitle="SSD1306 OLED"; init(OLED_ADAFRUIT_I2C_128x64, -1); }
  Adafruit_SSD1306(uint8_t slaveAddress, uint8_t OLED_TYPE=OLED_ADAFRUIT_I2C_128x64, int8_t rst_pin=-1) 
    : i2cDevice(slaveAddress)
    { fTitle="SSD1306 OLED"; init(OLED_TYPE, rst_pin); }
  
  ~Adafruit_SSD1306() { close(); }

  // I2C Init
  bool init(uint8_t OLED_TYPE, int8_t RST);

  void select_oled(uint8_t OLED_TYPE);
	
  void begin(void);
  void close(void);

  void ssd1306_command(uint8_t c);
  void ssd1306_command(uint8_t c0, uint8_t c1);
  void ssd1306_command(uint8_t c0, uint8_t c1, uint8_t c2);
  void ssd1306_data(uint8_t c);

  void clearDisplay(void);
  void invertDisplay(bool inv);
  void display();

  void startscrollright(uint8_t start, uint8_t stop);
  void startscrollleft(uint8_t start, uint8_t stop);

  void startscrolldiagright(uint8_t start, uint8_t stop);
  void startscrolldiagleft(uint8_t start, uint8_t stop);
  void stopscroll(void);

  void drawPixel(int16_t x, int16_t y, uint16_t color);

private:
  uint8_t *poledbuff=nullptr;	// Pointer to OLED data buffer in memory
  int8_t rst;
  int16_t ssd1306_lcdwidth, ssd1306_lcdheight;
  uint8_t vcc_type;
	
  void fastI2Cwrite(uint8_t c);
  void fastI2Cwrite(char* tbuf, uint32_t len);
};


#endif // _I2CDEVICES_H_
