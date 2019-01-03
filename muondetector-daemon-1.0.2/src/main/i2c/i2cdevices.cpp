#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>     // open
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h> // ioctl
#include <inttypes.h>  	    // uint8_t, etc
//#include <linux/i2c.h> // I2C bus definitions for linux like systems
//#include <linux/i2c-dev.h> // I2C bus definitions for linux like systems
#include <algorithm>
#include "i2cdevices.h"

#define DEFAULT_DEBUG_LEVEL 0

using namespace std;

unsigned int i2cDevice::fNrDevices = 0;
unsigned long int i2cDevice::fGlobalNrBytesRead = 0;
unsigned long int i2cDevice::fGlobalNrBytesWritten = 0;
std::vector<i2cDevice*> i2cDevice::fGlobalDeviceList;

const double ADS1115::PGAGAINS[6] = { 6.144, 4.096, 2.048, 1.024, 0.512, 0.256 };
const float MCP4728::VDD = 3.3;	// change, if device powered with different voltage
const double HMC5883::GAIN[8] = { 0.73, 0.92, 1.22, 1.52, 2.27, 2.56, 3.03, 4.35 };


i2cDevice::i2cDevice() {
	//opening the devicefile from the i2c driversection (open the device i2c)
	//"/dev/i2c-0" or "../i2c-1" for linux system. In our case 
	fHandle = open("/dev/i2c-1", O_RDWR);
	if (fHandle > 0) fNrDevices++;
	fNrBytesRead = 0;
	fNrBytesWritten = 0;
	fAddress = 0;
	fDebugLevel = DEFAULT_DEBUG_LEVEL;
	fGlobalDeviceList.push_back(this);
}

i2cDevice::i2cDevice(const char* busAddress = "/dev/i2c-1") {
	//opening the devicefile from the i2c driversection (open the device i2c)
	//"/dev/i2c-0" or "../i2c-1" for linux system. In our case 
	fHandle = open(busAddress, O_RDWR);
	if (fHandle > 0) fNrDevices++;
	fNrBytesRead = 0;
	fNrBytesWritten = 0;
	fAddress = 0;
	fDebugLevel = DEFAULT_DEBUG_LEVEL;
	fGlobalDeviceList.push_back(this);
}

i2cDevice::i2cDevice(uint8_t slaveAddress) : fAddress(slaveAddress) {
	//opening the devicefile from the i2c driversection (open the device i2c)
	//"/dev/i2c-0" or "../i2c-1" for linux system. In our case 
	fHandle = open("/dev/i2c-1", O_RDWR);
	ioctl(fHandle, I2C_SLAVE, fAddress);
	if (fHandle > 0) fNrDevices++;
	fNrBytesRead = 0;
	fNrBytesWritten = 0;
	fDebugLevel = DEFAULT_DEBUG_LEVEL;
	fGlobalDeviceList.push_back(this);
}

i2cDevice::i2cDevice(const char* busAddress, uint8_t slaveAddress) : fAddress(slaveAddress) {
	//opening the devicefile from the i2c driversection (open the device i2c)
	//"/dev/i2c-0" or "../i2c-1" for linux system. In our case 
	fHandle = open(busAddress, O_RDWR);
	ioctl(fHandle, I2C_SLAVE, fAddress);
	if (fHandle > 0) fNrDevices++;
	fNrBytesRead = 0;
	fNrBytesWritten = 0;
	fDebugLevel = DEFAULT_DEBUG_LEVEL;
	fGlobalDeviceList.push_back(this);
}

i2cDevice::~i2cDevice() {
	//destructor of the opening part from above
	if (fHandle > 0) fNrDevices--;
	close(fHandle);
	std::vector<i2cDevice*>::iterator it;
	it = std::find(fGlobalDeviceList.begin(), fGlobalDeviceList.end(), this);
	if (it!=fGlobalDeviceList.end()) fGlobalDeviceList.erase(it);
}

bool i2cDevice::devicePresent()
{
	uint8_t dummy;
	return readByte(0,&dummy);
}

void i2cDevice::setAddress(uint8_t address) {		        //pointer to our device on the i2c-bus
	fAddress = address;
	ioctl(fHandle, I2C_SLAVE, fAddress);	//i.g. Specify the address of the I2C Slave to communicate with
						//for example adress in our case "0x48" for the ads1115
}

int i2cDevice::read(uint8_t* buf, int nBytes) {		//defines a function with a pointer buf as buffer and the number of bytes which 
							//we want to read.
	int nread = ::read(fHandle, buf, nBytes);		//"::" declares that the functions does not call itself again, but instead
	if (nread > 0) {
		fNrBytesRead += nread;
		fGlobalNrBytesRead += nread;
	}
	return nread;				//uses the read funktion with the set parameters of the bool function
}

int i2cDevice::write(uint8_t* buf, int nBytes) {
	int nwritten = ::write(fHandle, buf, nBytes);
	if (nwritten > 0) {
		fNrBytesWritten += nwritten;
		fGlobalNrBytesWritten += nwritten;
	}
	return nwritten;
}

int i2cDevice::writeReg(uint8_t reg, uint8_t* buf, int nBytes)
{
	uint8_t writeBuf[nBytes + 1];

	writeBuf[0] = reg;		// first byte is register address
	for (int i = 0; i < nBytes; i++) writeBuf[i + 1] = buf[i];
	int n = write(writeBuf, nBytes + 1);
	return n - 1;

	//     if (length > 127) {
	//         fprintf(stderr, "Byte write count (%d) > 127\n", length);
	//         return(FALSE);
	//     }
	// 
	//     fd = open("/dev/i2c-1", O_RDWR);
	//     if (fd < 0) {
	//         fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
	//         return(FALSE);
	//     }
	//     if (ioctl(fd, I2C_SLAVE, devAddr) < 0) {
	//         fprintf(stderr, "Failed to select device: %s\n", strerror(errno));
	//         close(fd);
	//         return(FALSE);
	//     }
	//     buf[0] = regAddr;
	//     memcpy(buf+1,data,length);
	//     count = write(fd, buf, length+1);
	//     if (count < 0) {
	//         fprintf(stderr, "Failed to write device(%d): %s\n", count, ::strerror(errno));
	//         close(fd);
	//         return(FALSE);
	//     } else if (count != length+1) {
	//         fprintf(stderr, "Short write to device, expected %d, got %d\n", length+1, count);
	//         close(fd);
	//         return(FALSE);
	//     }
	//     close(fd);

}

int i2cDevice::readReg(uint8_t reg, uint8_t* buf, int nBytes)
{
	int n = write(&reg, 1);
	if (n != 1) return -1;
	n = read(buf, nBytes);
	return n;

	//     if (fd < 0) {
	//         fprintf(stderr, "Failed to open device: %s\n", strerror(errno));
	//         return(-1);
	//     }
	//     if (ioctl(fd, I2C_SLAVE, devAddr) < 0) {
	//         fprintf(stderr, "Failed to select device: %s\n", strerror(errno));
	//         close(fd);
	//         return(-1);
	//     }
	//     if (write(fd, &regAddr, 1) != 1) {
	//         fprintf(stderr, "Failed to write reg: %s\n", strerror(errno));
	//         close(fd);
	//         return(-1);
	//     }
	//     count = read(fd, data, length);
	//     if (count < 0) {
	//         fprintf(stderr, "Failed to read device(%d): %s\n", count, ::strerror(errno));
	//         close(fd);
	//         return(-1);
	//     } else if (count != length) {
	//         fprintf(stderr, "Short read  from device, expected %d, got %d\n", length, count);
	//         close(fd);
	//         return(-1);
	//     }
	//     close(fd);


}

/** Read a single bit from an 8-bit device register.
 * @param regAddr Register regAddr to read from
 * @param bitNum Bit position to read (0-7)
 * @param data Container for single bit value
 * @return Status of read operation (true = success)
 */
int8_t i2cDevice::readBit(uint8_t regAddr, uint8_t bitNum, uint8_t *data) {
	uint8_t b;
	uint8_t count = readReg(regAddr, &b, 1);
	*data = b & (1 << bitNum);
	return count;
}

/** Read multiple bits from an 8-bit device register.
 * @param regAddr Register regAddr to read from
 * @param bitStart First bit position to read (0-7)
 * @param length Number of bits to read (not more than 8)
 * @param data Container for right-aligned value (i.e. '101' read from any bitStart position will equal 0x05)
 * @return Status of read operation (true = success)
 */
int8_t i2cDevice::readBits(uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t *data) {
	// 01101001 read byte
	// 76543210 bit numbers
	//    xxx   args: bitStart=4, length=3
	//    010   masked
	//   -> 010 shifted
	uint8_t count, b;
	if ((count = readReg(regAddr, &b, 1)) != 0) {
		uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
		b &= mask;
		b >>= (bitStart - length + 1);
		*data = b;
	}
	return count;
}

/** Read single byte from an 8-bit device register.
 * @param regAddr Register regAddr to read from
 * @param data Container for byte value read from device
 * @return Status of read operation (true = success)
 */
bool i2cDevice::readByte(uint8_t regAddr, uint8_t *data) {
	return (readBytes(regAddr, 1, data) == 1);
}

/** Read multiple bytes from an 8-bit device register.
 * @param regAddr First register regAddr to read from
 * @param length Number of bytes to read
 * @param data Buffer to store read data in
 * @return Number of bytes read (-1 indicates failure)
 */
int16_t i2cDevice::readBytes(uint8_t regAddr, uint16_t length, uint8_t *data) {
	// not used?! int8_t count = 0;
//    int fd = open("/dev/i2c-1", O_RDWR);
	return readReg(regAddr, data, length);
}

/** write a single bit in an 8-bit device register.
 * @param regAddr Register regAddr to write to
 * @param bitNum Bit position to write (0-7)
 * @param data New bit value to write
 * @return Status of operation (true = success)
 */
bool i2cDevice::writeBit(uint8_t regAddr, uint8_t bitNum, uint8_t data) {
	uint8_t b;
	int n = readByte(regAddr, &b);
	if (n != 1) return false;
	b = (data != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
	return writeByte(regAddr, b);
}

/** Write multiple bits in an 8-bit device register.
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-7)
 * @param length Number of bits to write (not more than 8)
 * @param data Right-aligned value to write
 * @return Status of operation (true = success)
 */
bool i2cDevice::writeBits(uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data) {
	//      010 value to write
	// 76543210 bit numbers
	//    xxx   args: bitStart=4, length=3
	// 00011100 mask byte
	// 10101111 original value (sample)
	// 10100011 original & ~mask
	// 10101011 masked | value
	uint8_t b;
	if (readByte(regAddr, &b) != 0) {
		uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
		data <<= (bitStart - length + 1); // shift data into correct position
		data &= mask; // zero all non-important bits in data
		b &= ~(mask); // zero all important bits in existing byte
		b |= data; // combine data with existing byte
		return writeByte(regAddr, b);
	}
	else {
		return false;
	}
}

/** Write single byte to an 8-bit device register.
 * @param regAddr Register address to write to
 * @param data New byte value to write
 * @return Status of operation (true = success)
 */
bool i2cDevice::writeByte(uint8_t regAddr, uint8_t data) {
	return writeBytes(regAddr, 1, &data);
}

/** Write multiple bytes to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of bytes to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool i2cDevice::writeBytes(uint8_t regAddr, uint16_t length, uint8_t* data) {
	//     int8_t count = 0;
	//     uint8_t buf[128];
	//     int fd;

	int n = writeReg(regAddr, data, length);
	return (n == length);
}

/** Write multiple words to a 16-bit device register.
 * @param regAddr First register address to write to
 * @param length Number of words to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool i2cDevice::writeWords(uint8_t regAddr, uint16_t length, uint16_t* data) {
	int8_t count = 0;
	uint8_t buf[512];

	// Should do potential byteswap and call writeBytes() really, but that
	// messes with the callers buffer

	for (int i = 0; i < length; i++) {
		buf[i * 2] = data[i] >> 8;
		buf[i * 2 + 1] = data[i];
	}

	count = writeReg(regAddr, buf, length * 2);
	//    count = write(fd, buf, length*2+1);
	if (count < 0) {
		fprintf(stderr, "Failed to write device(%d): %s\n", count, ::strerror(errno));
		//        close(fd);
		return(false);
	}
	else if (count != length * 2) {
		fprintf(stderr, "Short write to device, expected %d, got %d\n", length + 1, count);
		//        close(fd);
		return(false);
	}
	//    close(fd);
	return true;
}

/** Write single word to a 16-bit device register.
 * @param regAddr Register address to write to
 * @param data New word value to write
 * @return Status of operation (true = success)
 */
bool i2cDevice::writeWord(uint8_t regAddr, uint16_t data) {
	return writeWords(regAddr, 1, &data);
}

void i2cDevice::startTimer() {
	gettimeofday(&fT1, NULL);
}

void i2cDevice::stopTimer() {
	gettimeofday(&fT2, NULL);
	// compute and print the elapsed time in millisec
	double elapsedTime = (fT2.tv_sec - fT1.tv_sec) * 1000.0;      // sec to ms
	elapsedTime += (fT2.tv_usec - fT1.tv_usec) / 1000.0;   // us to ms
	fLastTimeInterval = elapsedTime;
	if (fDebugLevel > 1)
		printf(" last transaction took: %6.2f ms\n", fLastTimeInterval);
}


/*
 * ADS1115 4ch 16 bit ADC
 */
int16_t ADS1115::readADC(unsigned int channel)
{
	uint8_t writeBuf[3];		// Buffer to store the 3 bytes that we write to the I2C device
	uint8_t readBuf[2];		// 2 byte buffer to store the data read from the I2C device  
	int16_t val;			// Stores the 16 bit value of our ADC conversion
  //  uint8_t fDataRate = 0x07; // 860 SPS
  //  uint8_t fDataRate = 0x01; // 16 SPS
	uint8_t fDataRate = fRate & 0x07;

	startTimer();

	// These three bytes are written to the ADS1115 to set the config register and start a conversion 
	writeBuf[0] = 0x01;		// This sets the pointer register so that the following two bytes write to the config register
	writeBuf[1] = 0x80;		// OS bit
	if (!fDiffMode) writeBuf[1] |= 0x40; // single ended mode channels
	writeBuf[1] |= (channel & 0x03) << 4; // channel select
	writeBuf[1] |= 0x01; // single shot mode
	writeBuf[1] |= ((uint8_t)fPga[channel]) << 1; // PGA gain select

	writeBuf[2] = 0x03;  		// This sets the 8 LSBs of the config register (bits 7-0)
	writeBuf[2] |= ((uint8_t)fDataRate) << 5;


	// Initialize the buffer used to read data from the ADS1115 to 0
	readBuf[0] = 0;
	readBuf[1] = 0;

	// Write writeBuf to the ADS1115, the 3 specifies the number of bytes we are writing,
	// this begins a single conversion	
	write(writeBuf, 3);

	// Wait for the conversion to complete, this requires bit 15 to change from 0->1
	int nloops = 0;
	while ((readBuf[0] & 0x80) == 0 && nloops*fReadWaitDelay / 1000 < 1000)	// readBuf[0] contains 8 MSBs of config register, AND with 10000000 to select bit 15
	{
		usleep(fReadWaitDelay);
		read(readBuf, 2);	// Read the config register into readBuf
		nloops++;
	}
	if (nloops*fReadWaitDelay / 1000 >= 1000) {
		if (fDebugLevel > 1)
			printf("timeout!\n");
		return -32767;
	}
	if (fDebugLevel > 2)
		printf(" nr of busy adc loops: %d \n", nloops);
	if (nloops > 1) {
		fReadWaitDelay += (nloops - 1)*fReadWaitDelay / 10;
		if (fDebugLevel > 1) {
			printf(" read wait delay: %6.2f ms\n", fReadWaitDelay / 1000.);
		}
	}
	//   else if (nloops==2) {
	//     fReadWaitDelay*=2.1;
	//   }
	  //else fReadWaitDelay--;

	  // Set pointer register to 0 to read from the conversion register
	readReg(0x00, readBuf, 2);		// Read the contents of the conversion register into readBuf

	val = readBuf[0] << 8 | readBuf[1];	// Combine the two bytes of readBuf into a single 16 bit result 
	fLastADCValue = val;

	stopTimer();
	fLastConvTime = fLastTimeInterval;

	return val;
}

bool ADS1115::devicePresent()
{
	uint8_t buf[2];
	return (read(buf, 2)==2);	// Read the config register into readBuf	
}

double ADS1115::readVoltage(unsigned int channel)
{
	double voltage = 0.;
	readVoltage(channel, voltage);
	return voltage;
}

void ADS1115::readVoltage(unsigned int channel, double& voltage)
{
	int16_t adc = 0;
	readVoltage(channel, adc, voltage);
}

void ADS1115::readVoltage(unsigned int channel, int16_t& adc, double& voltage)
{
	adc = readADC(channel);
	voltage = PGAGAINS[fPga[channel]] * adc / 32767.0;

	if (fAGC) {
		int eadc = abs(adc);
		if (eadc > 0.8 * 32767 && (unsigned int)fPga[channel] > 0) {
			fPga[channel] = CFG_PGA((unsigned int)fPga[channel] - 1);
			if (fDebugLevel > 1)
				printf("ADC input high...setting PGA to level %d\n", fPga[channel]);
		}
		else if (eadc < 0.2 * 32767 && (unsigned int)fPga[channel] < 5) {
			fPga[channel] = CFG_PGA((unsigned int)fPga[channel] + 1);
			if (fDebugLevel > 1)
				printf("ADC input low...setting PGA to level %d\n", fPga[channel]);

		}
	}
	fLastVoltage = voltage;
	return;
}


/*
 * MCP4728 4 ch 12 bit DAC
 */
bool MCP4728::setVoltage(uint8_t channel, float voltage, bool toEEPROM) {
	// Vout = (2.048V * Dn) / 4096 * Gx <= VDD
	if (voltage < 0) {
		return false;
	}
	uint8_t gain = 0;
	unsigned int value = (int)(voltage * 2000 + 0.5);
	if (value > 0xfff) {
		value = value >> 1;
		gain = 1;
	}
	if (value > 0xfff) {
		// error message
		return false;
	}
	return setValue(channel, value, gain, toEEPROM);
}

bool MCP4728::setValue(uint8_t channel, uint16_t value, uint8_t gain, bool toEEPROM) {
	if (value > 0xfff) {
		value = 0xfff;
		// error number of bits exceeding 12
		return false;
	}
	startTimer();
	channel = channel & 0x03;
	uint8_t buf[3];
	if (toEEPROM) {
		buf[0] = 0b01011001;
	}
	else {
		buf[0] = 0b01000001;
	}
	buf[0] = buf[0] | (channel << 1); // 01000/01011 (multiwrite/singlewrite command) DAC1 DAC0 (channel) UDAC bit =1
	buf[1] = 0b10000000 | (uint8_t)((value & 0xf00) >> 8); // Vref PD1 PD0 Gx (gain) D11 D10 D9 D8
	buf[1] = buf[1] | (gain << 4);
	buf[2] = (uint8_t)(value & 0xff); 	// D7 D6 D5 D4 D3 D2 D1 D0
	if (write(buf, 3) != 3) {
		// somehow did not write exact same amount of bytes as it should
		return false;
		stopTimer();
	}
	stopTimer();
	return true;
}

bool MCP4728::devicePresent()
{
	uint8_t buf[24];
	startTimer();
	// perform a read sequence of all registers as described in datasheet
	return (read(buf, 24) == 24);
	stopTimer();
}

bool MCP4728::readChannel(uint8_t channel, DacChannel& channelData)
{
	if (channel > 3) {
		// error: channel index exceeding 3
		return false;
	}
	
	startTimer();
	
	uint8_t buf[24];
	if (read(buf, 24) != 24) {
		// somehow did not read exact same amount of bytes as it should
		stopTimer();
		return false;
	}
	uint8_t offs=(channelData.eeprom==false)?1:4;
	channelData.vref=(buf[channel*6+offs]&0x80)?VREF_2V:VREF_VDD;
	channelData.pd=(buf[channel*6+offs]&0x60)>>5;
	channelData.gain=(buf[channel*6+offs]&0x10)?GAIN2:GAIN1;
	channelData.value=(uint16_t)(buf[channel*6+offs]&0x0f)<<8;
	channelData.value|=(uint16_t)(buf[channel*6+offs+1]&0xff);
	
	stopTimer();
	
	return true;
}

float MCP4728::code2voltage(const DacChannel& channelData)
{
	float vref =  (channelData.vref == VREF_2V)?2.048:VDD;
	float voltage = vref*channelData.value/4096;
	if (channelData.gain==GAIN2 && channelData.vref != VREF_VDD) voltage*=2.;
	return voltage;
}

/*
 * PCA9536 4 pin I/O expander
 */
bool PCA9536::setOutputPorts(uint8_t portMask) {
	unsigned char data = ~portMask;
	startTimer();
	if (1 != writeReg(CONFIG_REG, &data, 1)) {
		return false;
	}
	stopTimer();
	return true;
}

bool PCA9536::setOutputState(uint8_t portMask) {
	startTimer();
	if (1 != writeReg(OUTPUT_REG, &portMask, 1)) {
		return false;
	}
	stopTimer();
	return true;
}

uint8_t PCA9536::getInputState() {
	uint8_t inport=0x00;
	startTimer();
	readReg(INPUT_REG, &inport, 1);
	stopTimer();
	return inport & 0x0f;
}

bool PCA9536::devicePresent() {
	uint8_t inport=0x00;
	// read input port
	return (1==readReg(INPUT_REG, &inport, 1));
}

/*
 * LM75 Temperature Sensor
 */

int16_t LM75::readRaw()
{
	uint8_t readBuf[2];		// 2 byte buffer to store the data read from the I2C device  
	int16_t val;			// Stores the 16 bit value of our ADC conversion


	startTimer();

	readBuf[0] = 0;
	readBuf[1] = 0;

	read(readBuf, 2);	// Read the config register into readBuf

	//int8_t temperature0 = (int8_t)readBuf[0];

	// We extract the first bit and right shift 7 seven bit positions.
	// Why? Because we don't care about the bits 6,5,4,3,2,1 and 0.
//  int8_t temperature1 = (readBuf[1] & 0x80) >> 7; // is either zero or one
//	int8_t temperature1 = (readBuf[1] & 0xf8) >> 3;
	//int8_t temperature1 = (readBuf[1] & 0xff);

	//val = (temperature0 << 8) | temperature1;
	//val = readBuf[0] << 1 | (readBuf[1] >> 7);	// Combine the two bytes of readBuf into a single 16 bit result 
	val=((int16_t)readBuf[0] << 8) | readBuf[1];
	fLastRawValue = val;

	stopTimer();

	return val;
}

bool LM75::devicePresent()
{
	uint8_t readBuf[2];		// 2 byte buffer to store the data read from the I2C device  
	readBuf[0] = 0;
	readBuf[1] = 0;

	int n=read(readBuf, 2);	// Read the data register into readBuf
	return (n==2);
}

double LM75::getTemperature()
{
	return (double)readRaw() / 256.;
}


/*
 * X9119
 */

unsigned int X9119::readWiperReg()
{

	// just return the locally stored last value written to WCR
	// since readback doesn't work without repeated start transaction
	return fWiperReg;

	uint8_t writeBuf[3];		// Buffer to store the 3 bytes that we write to the I2C device
	uint8_t readBuf[16];		// 2 byte buffer to store the data read from the I2C device  
	int16_t val;			// Stores the 16 bit value of our ADC conversion

	writeBuf[0] = 0x80;		// op-code read WCR

	// Write writeBuf to the X9119
	// this sets the write address for WCR register and writes WCR
	write(writeBuf, 1);

	readBuf[0] = 0;
	readBuf[1] = 0;

	/*int n=*/read(readBuf, 2);	// Read the config register into readBuf
  //  printf( "%d bytes read\n",n);

	val = (readBuf[0] & 0x03) << 8 | readBuf[1];

	return val;
}

unsigned int X9119::readDataReg(uint8_t reg)
{

	uint8_t writeBuf[3];		// Buffer to store the 3 bytes that we write to the I2C device
	uint8_t readBuf[16];		// 2 byte buffer to store the data read from the I2C device  
	int16_t val;			// Stores the 16 bit value of our ADC conversion

	writeBuf[0] = 0x80 | 0x20 | (reg & 0x03) << 2;		// op-code read data reg

	// Write writeBuf to the X9119
	// this sets the write address for WCR register and writes WCR
	write(writeBuf, 1);

	readBuf[0] = 0;
	readBuf[1] = 0;

	/*int n=*/read(readBuf, 2);	// Read the config register into readBuf
  //  printf( "%d bytes read\n",n);

	val = (readBuf[0] & 0x03) << 8 | readBuf[1];

	return val;

}


unsigned int X9119::readWiperReg2()
{
	uint8_t writeBuf[3];		// Buffer to store the 3 bytes that we write to the I2C device
	uint8_t readBuf[16];		// 2 byte buffer to store the data read from the I2C device  
	int16_t val;			// Stores the 16 bit value of our ADC conversion

	writeBuf[0] = 0x80;		// op-code read WCR


	struct i2c_msg rdwr_msgs[2];

	rdwr_msgs[0].addr = fAddress;
	rdwr_msgs[0].flags = 0;
	rdwr_msgs[0].len = 1;
	rdwr_msgs[0].buf = (char*)writeBuf;

	rdwr_msgs[1].addr = fAddress;
	rdwr_msgs[1].flags = I2C_M_RD;
	rdwr_msgs[1].len = 2;
	rdwr_msgs[1].buf = (char*)readBuf;

	/*
	  = {
		{
		  .addr = fAddress,
		  .flags = 0, // write
		  .len = 1,
		  .buf = writeBuf
		},
		{ // Read buffer
		  .addr = fAddress,
		  .flags = I2C_M_RD, // read
		  .len = 2,
		  .buf = readBuf
		}
	  };*/

	struct i2c_rdwr_ioctl_data rdwr_data;

	rdwr_data.msgs = rdwr_msgs;
	rdwr_data.nmsgs = 2;

	//   = {
	//     .msgs = rdwr_msgs,
	//     .nmsgs = 2
	//   };


	  //::close(fHandle);
	  //fHandle = ::open( "/dev/i2c-1", O_RDWR );

	int result = ioctl(fHandle, I2C_RDWR, &rdwr_data);

	if (result < 0) {
		printf("rdwr ioctl error: %d\n", errno);
		perror("reason");
	}
	else {
		printf("rdwr ioctl OK\n");
		//    for ( i = 0; i < 16; ++i ) {
		//      printf( "%c", buffer[i] );
		//    }
		//    printf( "\n" );
	}


	val = (readBuf[0] & 0x03) << 8 | readBuf[1];

	return val;


	writeBuf[0] = 0x80;		// op-code read WCR

	// Write writeBuf to the X9119
	// this sets the write address for WCR register and writes WCR
	write(writeBuf, 1);

	readBuf[0] = 0;
	readBuf[1] = 0;

	read(readBuf, 2);	// Read the config register into readBuf

	val = (readBuf[0] & 0x03) << 8 | readBuf[1];

	return val;

}

unsigned int X9119::readWiperReg3()
{

	union i2c_smbus_data data;
	struct i2c_smbus_ioctl_data args;

	args.read_write = I2C_SMBUS_READ;
	args.command = 0x80;
	args.size = I2C_SMBUS_WORD_DATA;
	args.data = &data;
	int result = ioctl(fHandle, I2C_SMBUS, &args);
	if (result < 0) {
		printf("rdwr ioctl error: %d\n", errno);
		perror("reason");
		return 0;
	}
	else {
		printf("rdwr ioctl OK\n");
		//    for ( i = 0; i < 16; ++i ) {
		//      printf( "%c", buffer[i] );
		//    }
		//    printf( "\n" );
	}
	return 0x0FFFF & data.word;
}



void X9119::writeWiperReg(unsigned int value)
{
	uint8_t writeBuf[3];		// Buffer to store the 3 bytes that we write to the I2C device

	writeBuf[0] = 0x80 | 0x20;		// op-code write WCR
	writeBuf[1] = ((value & 0xff00) >> 8);	// MSB of WCR
	writeBuf[2] = (value & 0xff);		// LSB of WCR

	// Write writeBuf to the X9119
	// this sets the write address for WCR register and writes WCR
	int n = write(writeBuf, 3);
	//printf( "%d bytes written\n",n);
	if (n == 3) fWiperReg = value;

}

void X9119::writeDataReg(uint8_t reg, unsigned int value)
{
	uint8_t writeBuf[3];		// Buffer to store the 3 bytes that we write to the I2C device

	writeBuf[0] = 0x80 | 0x40 | (reg & 0x03) << 2;		// op-code write data reg
	writeBuf[1] = ((value & 0xff00) >> 8);	// MSB of WCR
	writeBuf[2] = (value & 0xff);		// LSB of WCR

	// Write writeBuf to the X9119
	// this sets the write address for data register and writes dr with value
	/*int n=*/write(writeBuf, 3);
	//  printf( "%d bytes written\n",n);

}




/*
 * 24AA02 EEPROM
 */
uint8_t EEPROM24AA02::readByte(uint8_t addr)
{
	uint8_t val = 0;
	startTimer();
	/*int n=*/readReg(addr, &val, 1);	// Read the data at address location
  //  printf( "%d bytes read\n",n);
	stopTimer();
	return val;
}

bool EEPROM24AA02::readByte(uint8_t addr, uint8_t* value)
{
	startTimer();
	int n=readReg(addr, value, 1);	// Read the data at address location
  //  printf( "%d bytes read\n",n);
	stopTimer();
	return (n==1);
}

/*
bool EEPROM24AA02::devicePresent()
{
	uint8_t dummy;
	return readByte(0,&dummy);
}
*/

void EEPROM24AA02::writeByte(uint8_t addr, uint8_t data)
{
	uint8_t writeBuf[2];		// Buffer to store the 2 bytes that we write to the I2C device

	writeBuf[0] = addr;		// address of data byte
	writeBuf[1] = data;		// data byte

	startTimer();

	// Write address first
	write(writeBuf, 2);

	usleep(5000);
	stopTimer();
}

/** Write multiple bytes to starting from given address into EEPROM memory.
 * @param addr First register address to write to
 * @param length Number of bytes to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 * @note this is an overloaded function to the one from the i2cdevice base class in order to 
 * prevent sequential write operations crossing page boundaries of the EEPROM. This function conforms to
 * the page-wise sequential write (c.f. http://ww1.microchip.com/downloads/en/devicedoc/21709c.pdf  p.7).
 */
bool EEPROM24AA02::writeBytes(uint8_t addr, uint16_t length, uint8_t* data) {

	static const uint8_t PAGESIZE=8;
	bool success = true;
	startTimer();
	for (uint16_t i=0; i<length; ) {
		uint8_t currAddr = addr+i;
		// determine, how many bytes left on current page
		uint8_t pageRemainder = PAGESIZE-currAddr%PAGESIZE;
		if (currAddr+pageRemainder>=length) pageRemainder = length-currAddr;
		int n = writeReg(currAddr, &data[i], pageRemainder);
		usleep(5000);
		i+=pageRemainder;
		success = success && (n==pageRemainder);
	}
	stopTimer();
	return success;
}



/*
* SHT21 Temperature&Humidity Sensor
* Prefered option is the no hold mastermode
*/


bool SHT21::checksumCorrect(uint8_t data[]) // expects data to be greater or equal 3 (expecting 3)
{
	const uint16_t Polynom = 0x131;
	uint8_t crc = 0;
	uint8_t byteCtr;
	uint8_t bit;
	try {
		for (byteCtr = 0; byteCtr < 2; ++byteCtr)
		{
			crc ^= (data[byteCtr]);
			for (bit = 8; bit > 0; --bit)
			{
				if (crc & 0x80)
				{
					crc = (crc << 1) ^ Polynom;
				}
				else
				{
					crc = (crc << 1);
				}
			}
		}
		if (crc != data[2])
		{
			return false;
		}
		else
		{
			return true;
		}
	}
	catch (int i) {
		printf("Error, Array too small in function: checksumCorrect\n");
		return false;
	}
}

float SHT21::getHumidity()
{
	uint16_t data_hum = readUH();
	float fhum;  //end calculation -> Humidity
	fhum = -6.0 + 125.0 * (((float)(data_hum)) / 65536.0);
	return (fhum);
}

float SHT21::getTemperature()
{
	uint16_t data_temp = readUT();
	float ftemp;  //endl calculation -> Temperature
	ftemp = -46.85 + 175.72 * (((float)(data_temp)) / 65536.0);
	return (ftemp);
}


uint16_t SHT21::readUT()  // von unsigned int auf float geändert
{
	uint8_t writeBuf[1];
	uint8_t readBuf[3];
	uint16_t data_temp;

	writeBuf[0] = 0;
	readBuf[0] = 0;
	readBuf[1] = 0;
	readBuf[2] = 0;
	data_temp = 0;

	writeReg(0xF3, writeBuf, 0);
	usleep(85000);
	while (read(readBuf, 3) != 3);

	if (fDebugLevel > 1)
	{
		printf("Inhalt: (MSB Byte): %x\n", readBuf[0]);
		printf("Inhalt: (LSB Byte): %x\n", readBuf[1]);
		printf("Inhalt: (Checksum Byte): %x\n", readBuf[2]);
	}

	data_temp = ((uint16_t)readBuf[0]) << 8;
	data_temp |= readBuf[1];
	data_temp &= 0xFFFC;  //Vergleich mit 0xFC um die letzten beiden Bits auf 00 zu setzen.

	if (!checksumCorrect(readBuf))
	{
		printf("checksum error\n");
	}
	return data_temp;
}

uint16_t SHT21::readUH()  //Hold mode
{
	uint8_t writeBuf[1];
	uint8_t readBuf[3];    //what should be read later
	uint16_t data_hum;

	writeBuf[0] = 0;
	readBuf[0] = 0;
	readBuf[1] = 0;
	readBuf[2] = 0;
	data_hum = 0;

	writeReg(0xF5, writeBuf, 0);
	usleep(30000);
	while (read(readBuf, 3) != 3);


	if (fDebugLevel>1)
	{
		printf("Es wurden gelesen (MSB Bytes): %x\n", readBuf[0]);
		printf("Es wurden gelesen (LSB Bytes): %x\n", readBuf[1]);
		printf("Es wurden gelesen (Checksum Bytes): %x\n", readBuf[2]);
	}

	data_hum = ((uint16_t)readBuf[0]) << 8;
	data_hum |= readBuf[1];
	data_hum &= 0xFFFC;  //Vergleich mit 0xFC um die letzten beiden Bits auf 00 zu setzen.

	if (!checksumCorrect(readBuf))
	{
		printf("checksum error\n");
	}
	return data_hum;
}



uint8_t SHT21::readResolutionSettings()
{  //reads the temperature and humidity resolution settings byte
	uint8_t readBuf[1]; 
	
	readBuf[0] = 0;  //Initialization

	readReg(0xE7, readBuf, 1);
	return readBuf[0];
}

void SHT21::setResolutionSettings(uint8_t settingsByte) 
{ //sets the temperature and humidity resolution settings byte
	uint8_t readBuf[1];
	readBuf[0] = 0;  //Initialization
	readReg(0xE7, readBuf, 1);
	readBuf[0] &= 0b00111000; // mask, to not change reserved bits (Bit 3, 4 and 5 are reserved)
	settingsByte &= 0b11000111;
	uint8_t writeBuf[1];
	writeBuf[0] = settingsByte | readBuf[0];
	writeReg(0xE6, writeBuf, 1);
}


bool SHT21::softReset()
{
	uint8_t writeBuf[1];
	writeBuf[0] = 0xFE;

	int n = 0;
	n = writeReg(0xFE, writeBuf, 0);
	usleep(15000);  //wait for the SHT to reset; datasheet on page 9

	if (n == 0)
	{
		printf("soft_reset succesfull %i\n", n);
	}

	return(n == 0);  //Wenn n == 0, gibt die Funktion True zurück. Wenn nicht gibt sie False zurück.


}

/*
 * BMP180 Pressure Sensor
 */

bool BMP180::init()
{
	uint8_t val = 0;

	fCalibrationValid = false;

	// chip id reg
	int n = readReg(0xd0, &val, 1);	// Read the id register into readBuf
  //  printf( "%d bytes read\n",n);

	if (fDebugLevel > 1)
	{
		printf("%d bytes read\n", n);
		printf("chip id: 0x%x \n", val);
	}
	if (val == 0x55) readCalibParameters();
	return (val == 0x55);
}


unsigned int BMP180::readUT()
{
	uint8_t readBuf[2];		// 2 byte buffer to store the data read from the I2C device  
	unsigned int val;		// Stores the 16 bit value of our ADC conversion

	// start temp measurement: CR -> 0x2e
	uint8_t cr_val = 0x2e;
	// register address control reg
	int n = writeReg(0xf4, &cr_val, 1);

	// wait at least 4.5 ms
	usleep(4500);

	readBuf[0] = 0;
	readBuf[1] = 0;

	// adc reg
	n = readReg(0xf6, readBuf, 2);	// Read the config register into readBuf
	if (fDebugLevel > 1)
		printf("%d bytes read\n", n);

	val = (readBuf[0]) << 8 | readBuf[1];

	return val;
}

unsigned int BMP180::readUP(uint8_t oss)
{
	uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  
	unsigned int val;			// Stores the 16 bit value of our ADC conversion
	static const int delay[4] = { 4500, 7500, 13500, 25500 };

	// start pressure measurement: CR -> 0x34 | oss<<6
	uint8_t cr_val = 0x34 | (oss & 0x03) << 6;
	// register address control reg
	int n = writeReg(0xf4, &cr_val, 1);
	usleep(delay[oss & 0x03]);

	//   writeBuf[0] = 0xf6;		// adc reg
	//   write(writeBuf, 1);

	readBuf[0] = 0;
	readBuf[1] = 0;
	readBuf[2] = 0;

	// adc reg
	n = readReg(0xf6, readBuf, 3);	// Read the conversion result into readBuf
	if (fDebugLevel > 1)
		printf("%d bytes read\n", n);

	val = (readBuf[0] << 16 | readBuf[1] << 8 | readBuf[2]) >> (8 - (oss & 0x03));

	return val;
}

signed int BMP180::getCalibParameter(unsigned int param) const
{
	if (param < 11) return fCalibParameters[param];
	return 0xffff;
}

void BMP180::readCalibParameters()
{
	uint8_t readBuf[22];
	// register address first byte eeprom
	int n = readReg(0xaa, readBuf, 22);	// Read the 11 eeprom word values into readBuf
	if (fDebugLevel > 1)
		printf("%d bytes read\n", n);

	if (fDebugLevel > 1)
		printf("BMP180 eeprom calib data:\n");

	bool ok = true;
	for (int i = 0; i < 11; i++) {
		if (i > 2 && i < 6)
			fCalibParameters[i] = (uint16_t)(readBuf[2 * i] << 8 | readBuf[2 * i + 1]);
		else
			fCalibParameters[i] = (int16_t)(readBuf[2 * i] << 8 | readBuf[2 * i + 1]);
		if (fCalibParameters[i] == 0 || fCalibParameters[i] == 0xffff) ok = false;
		if (fDebugLevel > 1)
			//      printf( "calib%d: 0x%4x \n",i,fCalibParameters[i]);
			printf("calib%d: %d \n", i, fCalibParameters[i]);
	}
	if (fDebugLevel > 1) {
		if (ok)
			printf("calib data is valid.\n");
		else printf("calib data NOT valid!\n");
	}

	fCalibrationValid = ok;
}

double BMP180::getTemperature() {
	if (!fCalibrationValid) return -999.;
	signed int UT = readUT();
	signed int X1 = ((UT - fCalibParameters[5])*fCalibParameters[4]) >> 15;
	signed int X2 = (fCalibParameters[9] << 11) / (X1 + fCalibParameters[10]);
	signed int B5 = X1 + X2;
	double T = (B5 + 8) / 160.;
	if (fDebugLevel > 1) {
		printf("UT=%d\n", UT);
		printf("X1=%d\n", X1);
		printf("X2=%d\n", X2);
		printf("B5=%d\n", B5);
		printf("Temp=%f\n", T);
	}
	return T;
}


double BMP180::getPressure(uint8_t oss) {
	if (!fCalibrationValid) return 0.;
	signed int UT = readUT();
	if (fDebugLevel > 1)
		printf("UT=%d\n", UT);
	signed int UP = readUP(oss);
	if (fDebugLevel > 1)
		printf("UP=%d\n", UP);
	signed int X1 = ((UT - fCalibParameters[5])*fCalibParameters[4]) >> 15;
	signed int X2 = (fCalibParameters[9] << 11) / (X1 + fCalibParameters[10]);
	signed int B5 = X1 + X2;
	signed int B6 = B5 - 4000;
	if (fDebugLevel > 1)
		printf("B6=%d\n", B6);
	X1 = (fCalibParameters[7] * ((B6*B6) >> 12)) >> 11;
	if (fDebugLevel > 1)
		printf("X1=%d\n", X1);
	X2 = (fCalibParameters[1] * B6) >> 11;
	if (fDebugLevel > 1)
		printf("X2=%d\n", X2);
	signed int X3 = X1 + X2;
	signed int B3 = ((fCalibParameters[0] * 4 + X3) << (oss & 0x03)) + 2;
	B3 = B3 / 4;
	if (fDebugLevel > 1)
		printf("B3=%d\n", B3);
	X1 = (fCalibParameters[2] * B6) >> 13;
	if (fDebugLevel > 1)
		printf("X1=%d\n", X1);
	X2 = (fCalibParameters[6] * (B6*B6) / 4096);
	X2 = X2 >> 16;
	if (fDebugLevel > 1)
		printf("X2=%d\n", X2);
	X3 = (X1 + X2 + 2) / 4;
	if (fDebugLevel > 1)
		printf("X3=%d\n", X3);
	unsigned long B4 = (fCalibParameters[3] * (unsigned long)(X3 + 32768)) >> 15;
	if (fDebugLevel > 1)
		printf("B4=%ld\n", B4);
	unsigned long B7 = ((unsigned long)UP - B3)*(50000 >> (oss & 0x03));
	if (fDebugLevel > 1)
		printf("B7=%ld\n", B7);
	int p = 0;
	if (B7 < 0x80000000) p = (B7 * 2) / B4;
	else p = (B7 / B4) * 2;
	if (fDebugLevel > 1)
		printf("p=%d\n", p);

	X1 = p >> 8;
	if (fDebugLevel > 1)
		printf("X1=%d\n", X1);
	X1 = X1 * X1;
	if (fDebugLevel > 1)
		printf("X1=%d\n", X1);
	X1 = (X1 * 3038) >> 16;
	if (fDebugLevel > 1)
		printf("X1=%d\n", X1);
	X2 = (-7357 * p) >> 16;
	if (fDebugLevel > 1)
		printf("X2=%d\n", X2);
	p = 16 * p + (X1 + X2 + 3791);
	double press = p / 16.;

	if (fDebugLevel > 1) {
		printf("pressure=%f\n", press);
	}
	return press;
}

/*
 *BME280 HumidityTemperaturePressuresensor
 *
*/
bool BME280::init()
{
	uint8_t val = 0;

	fCalibrationValid = false;

	// chip id reg
	int n = readReg(0xd0, &val, 1);	// Read the id register into readBuf
									//  printf( "%d bytes read\n",n);

	if (fDebugLevel > 1)
	{
		printf("%d bytes read\n", n);
		printf("chip id: 0x%x \n", val);
	}
	if (val == 0x60) readCalibParameters();
	return (val == 0x60);
}

unsigned int BME280::status() {
	uint8_t status[1];
	status[0] = 10;
	int n = readReg(0xf3, status, 1);
	if (fDebugLevel > 1)
		printf("%d bytes read\n", n);
	status[0] &= 0b1001;
	return (unsigned int)status[0];
}

unsigned int BME280::readConfig() {
	uint8_t config[1];
	config[0] = 0;
	int n = readReg(0xf5, config, 1);
	if (fDebugLevel > 1)
		printf("%d bytes read\n", n);
	return (unsigned int)config[0];
}

unsigned int BME280::read_CtrlMeasReg() {
	uint8_t ctrl_meas[1];
	ctrl_meas[0] = 0;
	int n = readReg(0xf4, ctrl_meas, 1);
	if (fDebugLevel > 1)
		printf("%d bytes read\n", n);
	return (unsigned int)ctrl_meas[0];
}

bool BME280::writeConfig(uint8_t config) {
	uint8_t buf[1];
	// check for bit 1 because datasheet says: "do not change"
	int n = readReg(0xf5, buf, 1);
	buf[0] = buf[0] & 0b10;
	config = config & 0b11111101;
	buf[0] = buf[0] | config;
	n += writeReg(0xf5, buf, 1);
	if (fDebugLevel > 1)
		printf("%d bytes written\n", n);
	return (n == 2);
}

bool BME280::write_CtrlMeasReg(uint8_t config) {
	uint8_t buf[1];
	buf[0] = config;
	int n = writeReg(0xf4, buf, 1);
	if (fDebugLevel > 1)
		printf("%d bytes written\n", n);
	return (n == 1);
}

bool BME280::setMode(uint8_t mode)
{	// 0= skipped; oversampling: 1: x1; 2:x2; 3: x4; 4:x8; 5:x16
	if (mode > 0b11) {
		if (fDebugLevel > 1)
			printf("mode > 3 error\n");
		return false;
	}
	uint8_t buf[1];
	int n = readReg(0xf4, buf, 1);
	uint8_t ctrl_meas = buf[0];
	ctrl_meas = ctrl_meas & 0xfc;
	ctrl_meas = ctrl_meas | mode;
	buf[0] = ctrl_meas;
	n += writeReg(0xf4, buf, 1);
	return (n == 2);
}

bool BME280::setTSamplingMode(uint8_t mode)
{	// 0= skipped; oversampling: 1: x1; 2:x2; 3: x4; 4:x8; 5:x16
	if (mode > 0b111) {
		if (fDebugLevel > 1)
			printf("mode > 3 error\n");
		return false;
	}
	uint8_t buf[1];
	int n = readReg(0xf4, buf, 1);
	uint8_t ctrl_meas = buf[0];
	ctrl_meas = ctrl_meas & 0b00011111;
	mode = mode << 5;
	ctrl_meas = ctrl_meas | mode;
	buf[0] = ctrl_meas;
	n += writeReg(0xf4, buf, 1);
	return (n == 2);
}

bool BME280::setPSamplingMode(uint8_t mode)
{	// 0= skipped; oversampling: 1: x1; 2:x2; 3: x4; 4:x8; 5:x16
	if (mode > 0b111) {
		if (fDebugLevel > 1)
			printf("mode > 3 error\n");
		return false;
	}
	uint8_t buf[1];
	int n = readReg(0xf4, buf, 1);
	uint8_t ctrl_meas = buf[0];
	ctrl_meas = ctrl_meas & 0b11100011;
	mode = mode << 2;
	ctrl_meas = ctrl_meas | mode;
	buf[0] = ctrl_meas;
	n += writeReg(0xf4, buf, 1);
	return (n == 2);
}

bool BME280::setHSamplingMode(uint8_t mode)
{	// 0= skipped; oversampling: 1: x1; 2:x2; 3: x4; 4:x8; 5:x16
	if (mode > 0b111) {
		if (fDebugLevel > 1)
			printf("mode > 3 error\n");
		return false;
	}
	uint8_t buf[1];
	int n = readReg(0xf2, buf, 1);
	uint8_t ctrl_meas = buf[0];
	ctrl_meas = ctrl_meas & 0b11111000;
	ctrl_meas = ctrl_meas | mode;
	buf[0] = ctrl_meas;
	n += writeReg(0xf2, buf, 1);
	return (n == 2);
}

void BME280::measure() {
	// calculate t_max [ms] from settings:
	uint8_t readBuf[1];
	double t_max = 1.25;
	readReg(0xf2, readBuf, 1);
	unsigned int val = readBuf[0] & 0b111;
	if (fDebugLevel > 1)
		printf("osrs_h: %u\n", val);
	if (val > 5)
		val = 5;
	unsigned int add = 1;
	if (val != 0) {
		add = add << (val - 1);
		t_max += 2.3*(double)add + 0.575;
	}

	readBuf[0] = 0;
	add = 1;
	readReg(0xf4, readBuf, 1);
	val = readBuf[0] & 0b00011100;
	val = val >> 2;
	if (fDebugLevel > 1)
		printf("osrs_p: %u\n", val);
	if (val > 5)
		val = 5;
	if (val != 0) {
		add = add << (val - 1);
		t_max += 2.3*(double)add + 0.575;
	}

	add = 1;
	val = readBuf[0] & 0b11100000;
	val = val >> 5;
	if (fDebugLevel > 1)
		printf("osrs_t: %u\n", val);
	if (val > 5)
		val = 5;
	if (val != 0) {
		add = add << (val - 1);
		t_max += 2.3*(double)add;
	}
	// t_max corresponds to the maximum time that a measurement can take with given
	// settings read out from registers f2 and f4

	// wait while status not ready:
	while (status() != 0) {
		usleep(5000);
	}
	setMode(0x2); // set mode to "forced measurement" (single-shot)
				  // it will now perform a measurement as configured in 0xf4, 0xf2 and 0xf5 registers

				  // wait at least 112.8 ms for a full accuracy measurement of all 3 values
				  // or ask for status to be 0
	usleep((int)(t_max * 1000 + 0.5) + 200);
	if (fDebugLevel > 1)
		printf("measurement took about %.1f ms\n", t_max + 0.2);
	while (status() != 0) {
		usleep(5000);
	}
	return;
}

unsigned int BME280::readUT()
{
	uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  
	uint32_t val;		// Stores the 20 bit value of our ADC conversion

	measure();

	readBuf[0] = 0;
	readBuf[1] = 0;
	readBuf[2] = 0;

	// adc reg
	int n = readReg(0xfa, readBuf, 3);	// Read the config register into readBuf
	if (fDebugLevel > 1)
		printf("%d bytes read: 0x%02x 0x%02x 0x%02x\n", n, readBuf[0], readBuf[1], readBuf[2]);

	val = ((uint32_t)readBuf[0]) << 12;			// <-------///// should the lower 4 bits or the higher 4 bits of the 24 bits of registers be left 0 ???
	val |= ((uint32_t)readBuf[1]) << 4;		// (look datasheet page 25)
	val |= ((uint32_t)readBuf[2]) >> 4;

	return val;
}

unsigned int BME280::readUP()
{
	uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  
	uint32_t val;		// Stores the 20 bit value of our ADC conversion

	measure();

	readBuf[0] = 0;
	readBuf[1] = 0;
	readBuf[2] = 0;

	// adc reg
	int n = readReg(0xf7, readBuf, 3);	// Read the config register into readBuf
	if (fDebugLevel > 1)
		printf("%d bytes read\n", n);

	val = ((uint32_t)readBuf[0]) << 12;
	val |= ((uint32_t)readBuf[1]) << 4;
	val |= ((uint32_t)readBuf[2]) >> 4;

	return val;
}

unsigned int BME280::readUH()
{
	uint8_t readBuf[2];
	uint16_t val;

	measure();
	readBuf[0] = 0;
	readBuf[1] = 0;

	// adc reg
	int n = readReg(0xfd, readBuf, 2);	// Read the config register into readBuf
	if (fDebugLevel > 1)
		printf("%d bytes read\n", n);

	val = ((uint32_t)readBuf[0]) << 8;
	val |= ((uint32_t)readBuf[1]);		// (look datasheet page 25)

	return val;
}

TPH BME280::readTPCU()
{
	uint8_t readBuf[8];
	for (int i = 0; i < 8; i++) {
		readBuf[i] = 0;
	}
	measure();
	int n = readReg(0xf7, readBuf, 8); // read T, P and H registers;
	if (fDebugLevel > 1)
		printf("%d bytes read\n", n);
	uint32_t adc_P = ((uint32_t)readBuf[0]) << 12;
	adc_P |= ((uint32_t)readBuf[1]) << 4;
	adc_P |= ((uint32_t)readBuf[2]) >> 4;
	uint32_t adc_T = ((uint32_t)readBuf[3]) << 12;
	adc_T |= ((uint32_t)readBuf[4]) << 4;
	adc_T |= ((uint32_t)readBuf[5]) >> 4;
	uint32_t adc_H = ((uint32_t)readBuf[6]) << 8;
	adc_H |= ((uint32_t)readBuf[7]);		// (look datasheet page 25)

	TPH val;
	val.adc_P = (int32_t)adc_P;
	val.adc_T = (int32_t)adc_T;
	val.adc_H = (int32_t)adc_H;
	return val;
}

bool BME280::softReset() {
	uint8_t resetWord[1];
	resetWord[0] = 0xb6;
	int val = writeReg(0xe0, resetWord, 1);
	return(val == 1);
}

signed int BME280::getCalibParameter(unsigned int param) const
{
	if (param < 11) return fCalibParameters[param];
	return 0xffff;
}

void BME280::readCalibParameters()
{
	uint8_t readBuf[33];
	uint8_t readBufPart2[8];
	// register address first byte eeprom
	int n = readReg(0x88, readBuf, 25);	// Read the 26 eeprom word values into readBuf 
	n = n + readReg(0xe1, readBufPart2, 8); // from two different locations
	for (int i = 0; i < 8; i++) {
		readBuf[i + 25] = readBufPart2[i];
	}
	if (fDebugLevel > 1)
		printf("%d bytes read\n", n);

	if (fDebugLevel > 1)
		printf("BME280 eeprom calib data:\n");

	bool ok = true;
	for (int i = 0; i < 12; i++) {
		fCalibParameters[i] = ((uint16_t)readBuf[2 * i]) | (((uint16_t)readBuf[2 * i + 1]) << 8); // 2x 8-Bit ergibt 16-Bit Wort
																								  //if (i == 0 || i == 3)
																								  //	fCalibParameters[i] = ((uint16_t)readBuf[2 * i]) | (((uint16_t)readBuf[2 * i + 1]) << 8); // 2x 8-Bit ergibt 16-Bit Wort
																								  //else
																								  //	fCalibParameters[i] = ((int16_t)readBuf[2 * i]) | (((int16_t)readBuf[2 * i + 1]) << 8);
		if (fCalibParameters[i] == 0 || fCalibParameters[i] == 0xffff) ok = false;
		if (fDebugLevel > 1)
			printf("calib%d: %d \n", i, fCalibParameters[i]);
	}
	fCalibParameters[12] = (uint16_t)readBuf[24];
	if (fDebugLevel > 1)
		printf("calib%d: %d \n", 12, fCalibParameters[12]);
	fCalibParameters[13] = ((uint16_t)readBuf[25]) | (((uint16_t)readBuf[26]) << 8);
	if (fDebugLevel > 1)
		printf("calib%d: %d \n", 13, fCalibParameters[13]);
	fCalibParameters[14] = (uint16_t)readBuf[27];
	if (fDebugLevel > 1)
		printf("calib%d: %d \n", 14, fCalibParameters[14]);
	fCalibParameters[15] = (((uint16_t)readBuf[28]) << 4) | (((uint16_t)readBuf[29]) & 0b1111);
	if (fDebugLevel > 1)
		printf("calib%d: %d \n", 15, fCalibParameters[15]);
	fCalibParameters[16] = (((uint16_t)readBuf[30] & 0xf0) >> 4) | (((uint16_t)readBuf[31]) << 4);
	if (fDebugLevel > 1)
		printf("calib%d: %d \n", 16, fCalibParameters[16]);
	fCalibParameters[17] = (uint16_t)readBuf[32];
	if (fDebugLevel > 1)
		printf("calib%d: %d \n", 17, fCalibParameters[17]);

	if (fDebugLevel > 1) {
		if (ok)
			printf("calib data is valid.\n");
		else printf("calib data NOT valid!\n");
	}

	fCalibrationValid = ok;
}

TPH BME280::getTPHValues() {
	TPH vals = readTPCU();
	vals.T = getTemperature(vals.adc_T);
	vals.P = getPressure(vals.adc_P);
	vals.H = getHumidity(vals.adc_H);
	return vals;
}

double BME280::getTemperature(signed int adc_T) {
	adc_T = (int32_t)adc_T;
	if (!fCalibrationValid) return -999.;
	uint32_t dig_T1 = (uint32_t)fCalibParameters[0];
	int32_t dig_T2 = (int32_t)((int16_t)fCalibParameters[1]);
	int32_t dig_T3 = (int32_t)((int16_t)fCalibParameters[2]);
	int32_t X1 = (((adc_T >> 3) - (((int32_t)dig_T1) << 1))*((int32_t)dig_T2)) >> 11;
	int32_t X2 = (((((adc_T >> 4) - ((int32_t)dig_T1))*((adc_T >> 4) - ((int32_t)dig_T1))) >> 12)*((int32_t)dig_T3)) >> 14;
	fT_fine = X1 + X2;
	int32_t t = (fT_fine * 5 + 128) >> 8;
	double T = t / 100.0;
	if (fDebugLevel > 1) {
		printf("adc_T=%d\n", adc_T);
		printf("X1=%d\n", X1);
		printf("X2=%d\n", X2);
		printf("t_fine=%d\n", fT_fine);
		printf("temp=%d\n", t);
	}
	return T;
}

double BME280::getHumidity(signed int adc_H) { // please only do this if "getTemperature()" has been executed before
	return getHumidity(adc_H, fT_fine);
}
double BME280::getHumidity(signed int adc_H, signed int t_fine) {
	adc_H = (int32_t)adc_H;
	if (!fCalibrationValid) return -999.;
	if (t_fine == 0) return -999.;
	uint32_t dig_H1 = (uint32_t)fCalibParameters[12];
	int32_t dig_H2 = (int32_t)((int16_t)fCalibParameters[13]);
	uint32_t dig_H3 = (uint32_t)fCalibParameters[14];
	int32_t dig_H4 = (int32_t)((int16_t)fCalibParameters[15]);
	int32_t dig_H5 = (int32_t)((int16_t)fCalibParameters[16]);
	int32_t dig_H6 = (int32_t)((int8_t)((uint8_t)fCalibParameters[17]));
	int32_t X1 = 0;
	X1 = (t_fine - (76800));
	X1 = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5)* X1)) +
		(16384)) >> 15) * (((((((X1 * ((int32_t)dig_H6)) >> 10) * (((X1 *
		((int32_t)dig_H3)) >> 11) + (32768))) >> 10) + (2097152)) *
			((int32_t)dig_H2) + 8192) >> 14));
	X1 = (X1 - (((((X1 >> 15) * (X1 >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4));
	X1 = (X1 < 0 ? 0 : X1);
	X1 = (X1 > 419430400 ? 419430400 : X1);
	unsigned int h = (uint32_t)(X1 >> 12);
	double H = h / 1024.0;
	if (fDebugLevel > 1) {
		printf("adc_H=%d\n", adc_H);
		printf("X1=%d\n", X1);
		printf("t_fine=%d\n", t_fine);
		printf("Humidity=%u\n", h);
	}
	return H;
}

double BME280::getPressure(signed int adc_P) {
	return getPressure(adc_P, fT_fine);
}
double BME280::getPressure(signed int adc_P, signed int t_fine) {
	if (!fCalibrationValid) return -999.0;
	if (fT_fine == 0) return -999.0;
	uint32_t dig_P1 = (uint32_t)fCalibParameters[3];
	int32_t dig_P2 = (int32_t)((int16_t)fCalibParameters[4]);
	int32_t dig_P3 = (int32_t)((int16_t)fCalibParameters[5]);
	int32_t dig_P4 = (int32_t)((int16_t)fCalibParameters[6]);
	int32_t dig_P5 = (int32_t)((int16_t)fCalibParameters[7]);
	int32_t dig_P6 = (int32_t)((int16_t)fCalibParameters[8]);
	int32_t dig_P7 = (int32_t)((int16_t)fCalibParameters[9]);
	int32_t dig_P8 = (int32_t)((int16_t)fCalibParameters[10]);
	int32_t dig_P9 = (int32_t)((int16_t)fCalibParameters[11]);

	int64_t X1, X2, p;
	X1 = ((int64_t)t_fine) - 128000;
	X2 = X1 * X1 * (int64_t)dig_P6;
	X2 = X2 + ((X1*(int64_t)dig_P5) << 17);
	X2 = X2 + (((int64_t)dig_P4) << 35);
	X1 = ((X1 * X1 * (int64_t)dig_P3) >> 8) + ((X1 * (int64_t)dig_P2) << 12);
	X1 = (((((int64_t)1) << 47) + X1))*((int64_t)dig_P1) >> 33;
	if (X1 == 0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576 - adc_P;
	p = (((p << 31) - X2) * 3125) / X1;
	X1 = (((int64_t)dig_P9) * (p >> 13) * (p >> 13)) >> 25;
	X2 = (((int64_t)dig_P8) * p) >> 19;
	p = ((p + X1 + X2) >> 8) + (((int64_t)dig_P7) << 4);

	double P = ((uint32_t)p) / 256.;
	if (fDebugLevel > 1) {
		printf("adc_P=%d\n", adc_P);
		printf("X1=%lld\n", X1);
		printf("X2=%lld\n", X2);
		printf("p=%lld\n", p);
		printf("P=%.3f\n", P);
	}
	return P;
}

/*
 * HMC5883 3 axis magnetic field sensor (Honeywell)
 */
bool HMC5883::init()
{
	uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  

	// init value 0 for gain
	fGain = 0;

	//   writeBuf[0] = 0x0a;		// chip id reg A
	//   write(writeBuf, 1);

	readBuf[0] = 0;
	readBuf[1] = 0;
	readBuf[2] = 0;

	//  int n=read(readBuf, 3);	// Read the id registers into readBuf
	  // chip id reg A: 0x0a
	int n = readReg(0x0a, readBuf, 3);	// Read the id registers into readBuf
  //  printf( "%d bytes read\n",n);

	if (fDebugLevel > 1)
	{
		printf("%d bytes read\n", n);
		printf("id reg A: 0x%x \n", readBuf[0]);
		printf("id reg B: 0x%x \n", readBuf[1]);
		printf("id reg C: 0x%x \n", readBuf[2]);
	}

	if (readBuf[0] != 0x48) return false;

	// set CRA
  //   writeBuf[0] = 0x00;		// addr config reg A (CRA)
  //   writeBuf[1] = 0x70;		// 8 average, 15 Hz, single measurement

	// addr config reg A (CRA)
	// 8 average, 15 Hz, single measurement: 0x70
	uint8_t cmd = 0x70;
	n = writeReg(0x00, &cmd, 1);

	setGain(fGain);
	return true;
}

void HMC5883::setGain(uint8_t gain)
{
	uint8_t writeBuf[3];		// Buffer to store the 3 bytes that we write to the I2C device

	// set CRB
	writeBuf[0] = 0x01;		// addr config reg B (CRB)
	writeBuf[1] = (gain & 0x07) << 5;	// gain=xx
	write(writeBuf, 2);
	fGain = gain & 0x07;
}

// uint8_t HMC5883::readGain()
// {
//   uint8_t writeBuf[3];		// Buffer to store the 3 bytes that we write to the I2C device
//   uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  
// 
//   // set CRB
//   writeBuf[0] = 0x01;		// addr config reg B (CRB)
//   write(writeBuf, 1);
// 
//   int n=read(readBuf, 1);	// Read the config register into readBuf
//   
//   if (n!=1) return 0;
//   uint8_t gain = readBuf[0]>>5;
//   if (fDebugLevel>1)
//   {
//     printf( "%d bytes read\n",n);
//     printf( "gain (read from device): 0x%x\n",gain);
//   }
//   //fGain = gain & 0x07;
//   return gain;
// }

uint8_t HMC5883::readGain()
{
	uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  

	int n = readReg(0x01, readBuf, 1);	// Read the config register into readBuf

	if (n != 1) return 0;
	uint8_t gain = readBuf[0] >> 5;
	if (fDebugLevel > 1)
	{
		printf("%d bytes read\n", n);
		printf("gain (read from device): 0x%x\n", gain);
	}
	//fGain = gain & 0x07;
	return gain;
}

bool HMC5883::readRDYBit()
{
	uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  

	// addr status reg (SR)
	int n = readReg(0x09, readBuf, 1);	// Read the status register into readBuf

	if (n != 1) return 0;
	uint8_t sr = readBuf[0];
	if (fDebugLevel > 1)
	{
		printf("%d bytes read\n", n);
		printf("status (read from device): 0x%x\n", sr);
	}
	if ((sr & 0x01) == 0x01) return true;
	return false;
}

bool HMC5883::readLockBit()
{
	uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  

	// addr status reg (SR)
	int n = readReg(0x09, readBuf, 1);	// Read the status register into readBuf

	if (n != 1) return 0;
	uint8_t sr = readBuf[0];
	if (fDebugLevel > 1)
	{
		printf("%d bytes read\n", n);
		printf("status (read from device): 0x%x\n", sr);
	}
	if ((sr & 0x02) == 0x02) return true;
	return false;
}

bool HMC5883::getXYZRawValues(int &x, int &y, int &z)
{
	uint8_t readBuf[6];

	uint8_t cmd = 0x01; // start single measurement
	int n = writeReg(0x02, &cmd, 1); // addr mode reg (MR)
	usleep(6000);

	// Read the 3 data registers into readBuf starting from addr 0x03
	n = readReg(0x03, readBuf, 6);
	int16_t xreg = (int16_t)(readBuf[0] << 8 | readBuf[1]);
	int16_t yreg = (int16_t)(readBuf[2] << 8 | readBuf[3]);
	int16_t zreg = (int16_t)(readBuf[4] << 8 | readBuf[5]);

	if (fDebugLevel > 1)
	{
		printf("%d bytes read\n", n);
		//    printf( "xreg: %2x  %2x  %d\n",readBuf[0], readBuf[1], xreg);
		printf("xreg: %d\n", xreg);
		printf("yreg: %d\n", yreg);
		printf("zreg: %d\n", zreg);


	}

	x = xreg; y = yreg; z = zreg;

	if (xreg >= -2048 && xreg < 2048 &&
		yreg >= -2048 && yreg < 2048 &&
		zreg >= -2048 && zreg < 2048)
		return true;

	return false;
}


bool HMC5883::getXYZMagneticFields(double &x, double &y, double &z)
{
	int xreg, yreg, zreg;
	bool ok = getXYZRawValues(xreg, yreg, zreg);
	double lsbgain = GAIN[fGain];
	x = lsbgain * xreg / 1000.;
	y = lsbgain * yreg / 1000.;
	z = lsbgain * zreg / 1000.;

	if (fDebugLevel > 1)
	{
		printf("x field: %f G\n", x);
		printf("y field: %f G\n", y);
		printf("z field: %f G\n", z);
	}

	return ok;
}

bool HMC5883::calibrate(int &x, int &y, int &z)
{

	// addr config reg A (CRA)
	// 8 average, 15 Hz, positive self test measurement: 0x71
	uint8_t cmd = 0x71;
	/*int n=*/writeReg(0x00, &cmd, 1);

	uint8_t oldGain = fGain;
	setGain(5);

	int xr, yr, zr;
	// one dummy measurement
	getXYZRawValues(xr, yr, zr);
	// measurement
	getXYZRawValues(xr, yr, zr);

	x = xr; y = yr; z = zr;

	setGain(oldGain);
	// one dummy measurement
	getXYZRawValues(xr, yr, zr);


	// set normal measurement mode in CRA again
	cmd = 0x70;
	/*n=*/writeReg(0x00, &cmd, 1);
	return true;
}
