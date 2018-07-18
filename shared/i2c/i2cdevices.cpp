#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>     // open
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h> // ioctl
#include <inttypes.h>  	    // uint8_t, etc
#include <linux/i2c.h> // I2C bus definitions for linux like systems
#include <linux/i2c-dev.h> // I2C bus definitions for linux like systems
#include "i2cdevices.h"

#define DEFAULT_DEBUG_LEVEL 0

using namespace std;

unsigned int i2cDevice::fNrDevices = 0;
unsigned long int i2cDevice::fGlobalNrBytesRead = 0;
unsigned long int i2cDevice::fGlobalNrBytesWritten = 0;


const double ADS1115::PGAGAINS[6]={6.144, 4.096, 2.048, 1.024, 0.512, 0.256};
const double HMC5883::GAIN[8]={ 0.73, 0.92, 1.22, 1.52, 2.27, 2.56, 3.03, 4.35 };


i2cDevice::i2cDevice() {	
	//opening the devicefile from the i2c driversection (open the device i2c)
	//"/dev/i2c-0" or "../i2c-1" for linux system. In our case 
	fHandle = open("/dev/i2c-1", O_RDWR);
	if (fHandle>0) fNrDevices++;
	fNrBytesRead=0;
	fNrBytesWritten=0;
	fAddress=0;
	fDebugLevel=DEFAULT_DEBUG_LEVEL;
}

i2cDevice::i2cDevice(const char* busAddress="/dev/i2c-1") {	
	//opening the devicefile from the i2c driversection (open the device i2c)
	//"/dev/i2c-0" or "../i2c-1" for linux system. In our case 
	fHandle = open(busAddress, O_RDWR);
	if (fHandle>0) fNrDevices++;
	fNrBytesRead=0;
	fNrBytesWritten=0;
	fAddress=0;
	fDebugLevel=DEFAULT_DEBUG_LEVEL;
}

i2cDevice::i2cDevice(uint8_t slaveAddress) : fAddress(slaveAddress) {	
	//opening the devicefile from the i2c driversection (open the device i2c)
	//"/dev/i2c-0" or "../i2c-1" for linux system. In our case 
	fHandle = open("/dev/i2c-1", O_RDWR);
	ioctl(fHandle, I2C_SLAVE, fAddress);
	if (fHandle>0) fNrDevices++;
	fNrBytesRead=0;
	fNrBytesWritten=0;
	fDebugLevel=DEFAULT_DEBUG_LEVEL;
}

i2cDevice::i2cDevice(const char* busAddress, uint8_t slaveAddress) : fAddress(slaveAddress) {
	//opening the devicefile from the i2c driversection (open the device i2c)
	//"/dev/i2c-0" or "../i2c-1" for linux system. In our case 
	fHandle = open(busAddress, O_RDWR);
	ioctl(fHandle, I2C_SLAVE, fAddress);
	if (fHandle>0) fNrDevices++;
	fNrBytesRead=0;
	fNrBytesWritten=0;
	fDebugLevel=DEFAULT_DEBUG_LEVEL;
}

i2cDevice::~i2cDevice() {
	//destructor of the opening part from above
	if (fHandle>0) fNrDevices--;
	close(fHandle);
}

void i2cDevice::setAddress(uint8_t address) {		        //pointer to our device on the i2c-bus
	fAddress=address;
	ioctl(fHandle, I2C_SLAVE, fAddress);	//i.g. Specify the address of the I2C Slave to communicate with
						//for example adress in our case "0x48" for the ads1115
}
				
int i2cDevice::read(uint8_t* buf, int nBytes) {		//defines a function with a pointer buf as buffer and the number of bytes which 
							//we want to read.
	int nread=::read(fHandle, buf, nBytes);		//"::" declares that the functions does not call itself again, but instead
	if (nread>0) {
		fNrBytesRead+=nread;
		fGlobalNrBytesRead+=nread;
	}
	return nread;				//uses the read funktion with the set parameters of the bool function
}

int i2cDevice::write(uint8_t* buf, int nBytes) {
	int nwritten=::write(fHandle, buf, nBytes);
	if (nwritten>0) {
		fNrBytesWritten+=nwritten;
		fGlobalNrBytesWritten+=nwritten;
	}
	return nwritten;
}

int i2cDevice::writeReg(uint8_t reg, uint8_t* buf, int nBytes)
{
  uint8_t writeBuf[nBytes+1];

  writeBuf[0] = reg;		// first byte is register address
  for (int i=0; i<nBytes; i++) writeBuf[i+1]=buf[i];
  int n=write(writeBuf, nBytes+1);
  return n-1;

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
  int n=write(&reg, 1);
  if (n!=1) return -1;
  n=read(buf, nBytes);
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
int8_t i2cDevice::readByte(uint8_t regAddr, uint8_t *data) {
    return readBytes(regAddr, 1, data);
}

/** Read multiple bytes from an 8-bit device register.
 * @param regAddr First register regAddr to read from
 * @param length Number of bytes to read
 * @param data Buffer to store read data in
 * @return Number of bytes read (-1 indicates failure)
 */
int8_t i2cDevice::readBytes(uint8_t regAddr, uint8_t length, uint8_t *data) {
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
    int n=readByte(regAddr, &b);
    if (n!=1) return false;
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
    } else {
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
bool i2cDevice::writeBytes(uint8_t regAddr, uint8_t length, uint8_t* data) {
//     int8_t count = 0;
//     uint8_t buf[128];
//     int fd;

    int n=writeReg(regAddr, data, length);
    return n==length;
}

/** Write multiple words to a 16-bit device register.
 * @param regAddr First register address to write to
 * @param length Number of words to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool i2cDevice::writeWords(uint8_t regAddr, uint8_t length, uint16_t* data) {
    int8_t count = 0;
    uint8_t buf[128];

    // Should do potential byteswap and call writeBytes() really, but that
    // messes with the callers buffer

    for (int i = 0; i < length; i++) {
        buf[i*2] = data[i] >> 8;
        buf[i*2+1] = data[i];
    }
    
    count=writeReg(regAddr, buf, length*2);
//    count = write(fd, buf, length*2+1);
    if (count < 0) {
        fprintf(stderr, "Failed to write device(%d): %s\n", count, ::strerror(errno));
//        close(fd);
        return(false);
    } else if (count != length*2) {
        fprintf(stderr, "Short write to device, expected %d, got %d\n", length+1, count);
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
  if (fDebugLevel>1)
    printf( " last transaction took: %6.2f ms\n", fLastTimeInterval );
}



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
  writeBuf[0] = 1;		// This sets the pointer register so that the following two bytes write to the config register
  writeBuf[1] = 0x80 | 0x40; // OS bit, single ended mode channels
  writeBuf[1] |= (channel & 0x03) << 4; // channel select
  writeBuf[1] |= 0x01; // single shot mode
  writeBuf[1] |= ((uint8_t)fPga[channel]) << 1; // PGA gain select

  writeBuf[2] = 0x03;  		// This sets the 8 LSBs of the config register (bits 7-0)
  writeBuf[2] |= ((uint8_t)fDataRate) << 5;
  
  
  // Initialize the buffer used to read data from the ADS1115 to 0
  readBuf[0]= 0;		
  readBuf[1]= 0;
	  
  // Write writeBuf to the ADS1115, the 3 specifies the number of bytes we are writing,
  // this begins a single conversion	
  write(writeBuf, 3);

  // Wait for the conversion to complete, this requires bit 15 to change from 0->1
  int nloops=0;
  while ((readBuf[0] & 0x80) == 0 && nloops*fReadWaitDelay/1000<1000)	// readBuf[0] contains 8 MSBs of config register, AND with 10000000 to select bit 15
  {
	usleep(fReadWaitDelay);
	read(readBuf, 2);	// Read the config register into readBuf
	nloops++;
  }
  if (nloops*fReadWaitDelay/1000>=1000) {
    if (fDebugLevel>1)
      printf( "timeout!\n" );
    return -32767;
  }
  if (fDebugLevel>2)
    printf( " nr of busy adc loops: %d \n", nloops );
  if (nloops>1) {
    fReadWaitDelay+=(nloops-1)*fReadWaitDelay/10;
    if (fDebugLevel>1) {
      printf( " read wait delay: %6.2f ms\n", fReadWaitDelay/1000. );
    }
  }
//   else if (nloops==2) {
//     fReadWaitDelay*=2.1;
//   }
  //else fReadWaitDelay--;

  // Set pointer register to 0 to read from the conversion register
  readReg(0x00, readBuf, 2);		// Read the contents of the conversion register into readBuf

  val = readBuf[0] << 8 | readBuf[1];	// Combine the two bytes of readBuf into a single 16 bit result 
  fLastADCValue=val;

  stopTimer();

  return val;
}

double ADS1115::readVoltage(unsigned int channel)
{
  double voltage=0.;
  readVoltage(channel, voltage);
  return voltage;
}

void ADS1115::readVoltage(unsigned int channel, double& voltage)
{
  int16_t adc=0;
  readVoltage(channel, adc, voltage);
}

void ADS1115::readVoltage(unsigned int channel, int16_t& adc, double& voltage)
{
  adc=readADC(channel);
  voltage=PGAGAINS[fPga[channel]]*adc/32767.0;

  if (fAGC) {
    int eadc=abs(adc);
    if (eadc>0.8*32767 && (unsigned int)fPga[channel]>0) {
      fPga[channel]=CFG_PGA((unsigned int)fPga[channel]-1);
      if (fDebugLevel>1)
	printf("ADC input high...setting PGA to level %d\n", fPga[channel]);
    } else if (eadc<0.2*32767 && (unsigned int)fPga[channel]<5) {
      fPga[channel]=CFG_PGA((unsigned int)fPga[channel]+1);
      if (fDebugLevel>1)
	printf("ADC input low...setting PGA to level %d\n", fPga[channel]);

    }
  }
  fLastVoltage=voltage;
  return;
}

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
	channel = channel & 3;
	uint8_t buf[3];
	if (toEEPROM) {
		buf[0] = 0b01011001;
	}
	else {
		buf[0] = 0b01000001;
	}
	buf[0] = buf[0] | (channel << 1); // 01000/01011 (multiwrite/singlewrite command) DAC1 DAC0 (channel) UDAC bit =1
	buf[1] = 0b10000000 | (uint8_t)((value & 0xf00) >> 8) ; // Vref PD1 PD0 Gx (gain) D11 D10 D9 D8
	buf[1] = buf[1] | (gain << 4);
	buf[2] = (uint8_t)(value & 0xff); 	// D7 D6 D5 D4 D3 D2 D1 D0
	if (write(buf, 3) != 3){
		// somehow did not write exact same amount of bytes as it should
		return false;
	}
	return true;
}

bool PCA9536::setOutputPorts(uint8_t portMask) { 
	unsigned char data = ~portMask;
	if (1 != writeReg(CONFIG_REG, &data, 1)) {
		return false;
    }
    return true;
}

bool PCA9536::setOutputState(uint8_t portMask) {
	if (1 != writeReg(OUTPUT_REG, &portMask, 1)) {
		return false;
	}
    return true;
}

/*
 * LM75 Temperature Sensor
 */

signed int LM75::readRaw()
{
  uint8_t readBuf[2];		// 2 byte buffer to store the data read from the I2C device  
  signed int val;			// Stores the 16 bit value of our ADC conversion
  

  startTimer();

  readBuf[0]= 0;		
  readBuf[1]= 0;
	  
  read(readBuf, 2);	// Read the config register into readBuf

  int8_t temperature0 = (int8_t) readBuf[0];

	// We extract the first bit and right shift 7 seven bit positions.
	// Why? Because we don't care about the bits 6,5,4,3,2,1 and 0.
//  int8_t temperature1 = (readBuf[1] & 0x80) >> 7; // is either zero or one
  int8_t temperature1 = (readBuf[1] & 0xf8)>>3;

  val = (temperature0 << 5) | temperature1;
  //val = readBuf[0] << 1 | (readBuf[1] >> 7);	// Combine the two bytes of readBuf into a single 16 bit result 
  fLastRawValue=val;

  stopTimer();

  return val;
}

double LM75::getTemperature()
{
  return (double)readRaw()/32.;
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

  readBuf[0]= 0;		
  readBuf[1]= 0;
	  
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

  writeBuf[0] = 0x80 | 0x20 | (reg & 0x03)<<2;		// op-code read data reg
  
  // Write writeBuf to the X9119
  // this sets the write address for WCR register and writes WCR
  write(writeBuf, 1);

  readBuf[0]= 0;		
  readBuf[1]= 0;
	  
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

  rdwr_msgs[0].addr=fAddress;
  rdwr_msgs[0].flags=0;
  rdwr_msgs[0].len=1;
  rdwr_msgs[0].buf=writeBuf;

  rdwr_msgs[1].addr=fAddress;
  rdwr_msgs[1].flags=I2C_M_RD;
  rdwr_msgs[1].len=2;
  rdwr_msgs[1].buf=readBuf;
  
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

  rdwr_data.msgs=rdwr_msgs;
  rdwr_data.nmsgs=2;
  
//   = {
//     .msgs = rdwr_msgs,
//     .nmsgs = 2
//   };

  
  //::close(fHandle);
  //fHandle = ::open( "/dev/i2c-1", O_RDWR );

  int result = ioctl( fHandle, I2C_RDWR, &rdwr_data );

  if ( result < 0 ) {
    printf( "rdwr ioctl error: %d\n", errno );
    perror( "reason" );
  } else {
    printf( "rdwr ioctl OK\n" );
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

  readBuf[0]= 0;		
  readBuf[1]= 0;
	  
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
  int result=ioctl(fHandle,I2C_SMBUS,&args);
  if ( result < 0 ) {
    printf( "rdwr ioctl error: %d\n", errno );
    perror( "reason" );
    return 0;
  } else {
    printf( "rdwr ioctl OK\n" );
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
  int n=write(writeBuf, 3);
  //printf( "%d bytes written\n",n);
  if (n==3) fWiperReg = value;
  
}

void X9119::writeDataReg(uint8_t reg, unsigned int value)
{
  uint8_t writeBuf[3];		// Buffer to store the 3 bytes that we write to the I2C device
  
  writeBuf[0] = 0x80 | 0x40 | (reg & 0x03)<<2;		// op-code write data reg
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
  uint8_t val=0;
  /*int n=*/readReg(addr, &val, 1);	// Read the data at address location
//  printf( "%d bytes read\n",n);
  return val;
}


void EEPROM24AA02::writeByte(uint8_t addr, uint8_t data)
{
  uint8_t writeBuf[2];		// Buffer to store the 3 bytes that we write to the I2C device
//  uint8_t readBuf[16];		// 2 byte buffer to store the data read from the I2C device  
//  uint8_t val;			// Stores the 16 bit value of our ADC conversion

  writeBuf[0] = addr;		// address of data byte
  writeBuf[1] = data;		// data byte

  startTimer();
  
  // Write address first
  write(writeBuf, 2);

  usleep(5000);
  stopTimer();

//  readBuf[0]= 0;		
	  
//  int n=read(&val, 1);	// Read the data at address location
//  printf( "%d bytes read\n",n);
  
//  val = readBuf[0];

//  return val;
}



/*
 * BMP180 Pressure Sensor
 */

bool BMP180::init()
{
  uint8_t val=0;

  fCalibrationValid=false;

  // chip id reg
  int n=readReg(0xd0, &val, 1);	// Read the id register into readBuf
//  printf( "%d bytes read\n",n);
  
  if (fDebugLevel>1)
  {
    printf( "%d bytes read\n",n);
    printf( "chip id: 0x%x \n",val);
  }
  if (val==0x55) readCalibParameters();
  return (val==0x55);
}


unsigned int BMP180::readUT()
{
  uint8_t readBuf[2];		// 2 byte buffer to store the data read from the I2C device  
  unsigned int val;		// Stores the 16 bit value of our ADC conversion

  // start temp measurement: CR -> 0x2e
  uint8_t cr_val = 0x2e;
  // register address control reg
  int n=writeReg(0xf4, &cr_val, 1); 

  // wait at least 4.5 ms
  usleep(4500);

  readBuf[0]= 0;		
  readBuf[1]= 0;
	  
  // adc reg
  n=readReg(0xf6, readBuf, 2);	// Read the config register into readBuf
  if (fDebugLevel>1)
    printf( "%d bytes read\n",n);
  
  val = (readBuf[0]) << 8 | readBuf[1];

  return val;
}

unsigned int BMP180::readUP(uint8_t oss)
{
  uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  
  unsigned int val;			// Stores the 16 bit value of our ADC conversion
  static const int delay[4] = { 4500, 7500, 13500, 25500};
  
  // start pressure measurement: CR -> 0x34 | oss<<6
  uint8_t cr_val = 0x34 | (oss & 0x03)<<6;
  // register address control reg
  int n=writeReg(0xf4, &cr_val, 1); 
  usleep(delay[oss & 0x03]);

//   writeBuf[0] = 0xf6;		// adc reg
//   write(writeBuf, 1);

  readBuf[0]= 0;
  readBuf[1]= 0;
  readBuf[2]= 0;

  // adc reg
  n=readReg(0xf6, readBuf, 3);	// Read the conversion result into readBuf
  if (fDebugLevel>1)
    printf( "%d bytes read\n",n);
  
  val = (readBuf[0] << 16 | readBuf[1] << 8 | readBuf[2]) >> (8-(oss & 0x03));

  return val;
}

signed int BMP180::getCalibParameter(unsigned int param) const
{
  if (param<11) return fCalibParameters[param];
  return 0xffff;
}

void BMP180::readCalibParameters()
{
  uint8_t readBuf[22];
  // register address first byte eeprom
  int n=readReg(0xaa, readBuf, 22);	// Read the 11 eeprom word values into readBuf
  if (fDebugLevel>1)
    printf( "%d bytes read\n",n);
  
  if (fDebugLevel>1)
    printf( "BMP180 eeprom calib data:\n");
  
  bool ok=true;
  for (int i=0; i<11; i++) {
    if (i>2 && i<6)
      fCalibParameters[i] = (uint16_t)(readBuf[2*i]<<8 | readBuf[2*i+1]);
    else
      fCalibParameters[i] = (int16_t)(readBuf[2*i]<<8 | readBuf[2*i+1]);
    if (fCalibParameters[i]==0 || fCalibParameters[i]==0xffff) ok=false;
    if (fDebugLevel>1)
//      printf( "calib%d: 0x%4x \n",i,fCalibParameters[i]);
      printf( "calib%d: %d \n",i,fCalibParameters[i]);
  }
  if (fDebugLevel>1) {
    if (ok)
      printf( "calib data is valid.\n");
    else printf( "calib data NOT valid!\n");
  }
  
  fCalibrationValid=ok;
}

double BMP180::getTemperature() {
  if (!fCalibrationValid) return -999.;
  signed int UT=readUT();
  signed int X1 = ((UT-fCalibParameters[5])*fCalibParameters[4])>>15;
  signed int X2 = (fCalibParameters[9]<<11)/(X1+fCalibParameters[10]);
  signed int B5 = X1+X2;
  double T=(B5 + 8)/160.;
  if (fDebugLevel>1) {
    printf( "UT=%d\n",UT);
    printf( "X1=%d\n",X1);
    printf( "X2=%d\n",X2);
    printf( "B5=%d\n",B5);
    printf( "Temp=%f\n",T);
  }  
  return T;
}


double BMP180::getPressure(uint8_t oss) {
  if (!fCalibrationValid) return 0.;
  signed int UT=readUT();
  if (fDebugLevel>1)
    printf( "UT=%d\n",UT);
  signed int UP=readUP(oss);
  if (fDebugLevel>1)
    printf( "UP=%d\n",UP);
  signed int X1 = ((UT-fCalibParameters[5])*fCalibParameters[4])>>15;
  signed int X2 = (fCalibParameters[9]<<11)/(X1+fCalibParameters[10]);
  signed int B5 = X1+X2;
  signed int B6 = B5-4000;
  if (fDebugLevel>1)
    printf( "B6=%d\n",B6);
  X1 = (fCalibParameters[7]*((B6*B6)>>12))>>11;
  if (fDebugLevel>1)
    printf( "X1=%d\n",X1);
  X2 = (fCalibParameters[1]*B6)>>11;
  if (fDebugLevel>1)
    printf( "X2=%d\n",X2);
  signed int X3 = X1+X2;
  signed int B3 = ((fCalibParameters[0]*4+X3)<<(oss & 0x03))+2;
  B3 = B3/4;
  if (fDebugLevel>1)
    printf( "B3=%d\n",B3);
  X1 = (fCalibParameters[2]*B6)>>13;
  if (fDebugLevel>1)
    printf( "X1=%d\n",X1);
  X2 = (fCalibParameters[6]*(B6*B6)/4096);
  X2 = X2 >> 16;
  if (fDebugLevel>1)
    printf( "X2=%d\n",X2);
  X3 = (X1+X2+2)/4;
  if (fDebugLevel>1)
    printf( "X3=%d\n",X3);
  unsigned long B4 = (fCalibParameters[3]*(unsigned long)(X3+32768))>>15;
  if (fDebugLevel>1)
    printf( "B4=%ld\n",B4);
  unsigned long B7 = ((unsigned long)UP-B3)*(50000>>(oss & 0x03));
  if (fDebugLevel>1)
    printf( "B7=%ld\n",B7);
  int p=0;
  if (B7 < 0x80000000) p=(B7*2)/B4;
  else p=(B7/B4)*2;
  if (fDebugLevel>1)
    printf( "p=%d\n",p);

  X1 = p>>8;
  if (fDebugLevel>1)
    printf( "X1=%d\n",X1);
  X1 = X1*X1;
  if (fDebugLevel>1)
    printf( "X1=%d\n",X1);
  X1 = (X1*3038)>>16;
  if (fDebugLevel>1)
    printf( "X1=%d\n",X1);
  X2 = (-7357*p)>>16;
  if (fDebugLevel>1)
    printf( "X2=%d\n",X2);
  p = 16*p + (X1+X2+3791);
  double press = p/16.;
  
  if (fDebugLevel>1) {
    printf( "pressure=%f\n",press);
  }  
  return press;
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

  readBuf[0]= 0;
  readBuf[1]= 0;
  readBuf[2]= 0;

//  int n=read(readBuf, 3);	// Read the id registers into readBuf
  // chip id reg A: 0x0a
  int n=readReg(0x0a, readBuf, 3);	// Read the id registers into readBuf
//  printf( "%d bytes read\n",n);
  
  if (fDebugLevel>1)
  {
    printf( "%d bytes read\n",n);
    printf( "id reg A: 0x%x \n",readBuf[0]);
    printf( "id reg B: 0x%x \n",readBuf[1]);
    printf( "id reg C: 0x%x \n",readBuf[2]);
  }

  if (readBuf[0]!=0x48) return false;

  // set CRA
//   writeBuf[0] = 0x00;		// addr config reg A (CRA)
//   writeBuf[1] = 0x70;		// 8 average, 15 Hz, single measurement
  
  // addr config reg A (CRA)
  // 8 average, 15 Hz, single measurement: 0x70
  uint8_t cmd = 0x70;
  n=writeReg(0x00, &cmd, 1);
  
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

  int n=readReg(0x01, readBuf, 1);	// Read the config register into readBuf
  
  if (n!=1) return 0;
  uint8_t gain = readBuf[0]>>5;
  if (fDebugLevel>1)
  {
    printf( "%d bytes read\n",n);
    printf( "gain (read from device): 0x%x\n",gain);
  }
  //fGain = gain & 0x07;
  return gain;
}

bool HMC5883::readRDYBit()
{
  uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  
  
  // addr status reg (SR)
  int n=readReg(0x09, readBuf, 1);	// Read the status register into readBuf
  
  if (n!=1) return 0;
  uint8_t sr = readBuf[0];
  if (fDebugLevel>1)
  {
    printf( "%d bytes read\n",n);
    printf( "status (read from device): 0x%x\n",sr);
  }
  if ((sr & 0x01) == 0x01) return true;
  return false;
}

bool HMC5883::readLockBit()
{
  uint8_t readBuf[3];		// 2 byte buffer to store the data read from the I2C device  
  
  // addr status reg (SR)
  int n=readReg(0x09, readBuf, 1);	// Read the status register into readBuf
  
  if (n!=1) return 0;
  uint8_t sr = readBuf[0];
  if (fDebugLevel>1)
  {
    printf( "%d bytes read\n",n);
    printf( "status (read from device): 0x%x\n",sr);
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
  n=readReg(0x03, readBuf, 6);
  int16_t xreg = (int16_t)(readBuf[0]<<8 | readBuf[1]);
  int16_t yreg = (int16_t)(readBuf[2]<<8 | readBuf[3]);
  int16_t zreg = (int16_t)(readBuf[4]<<8 | readBuf[5]);
  
  if (fDebugLevel>1)
  {
    printf( "%d bytes read\n",n);
//    printf( "xreg: %2x  %2x  %d\n",readBuf[0], readBuf[1], xreg);
    printf( "xreg: %d\n",xreg);
    printf( "yreg: %d\n",yreg);
    printf( "zreg: %d\n",zreg);
    
    
  }  
  
  x=xreg; y=yreg; z=zreg;
  
  if (xreg>=-2048 && xreg<2048 &&
      yreg>=-2048 && yreg<2048 &&
      zreg>=-2048 && zreg<2048)
  return true;
    
  return false;
}


bool HMC5883::getXYZMagneticFields(double &x, double &y, double &z)
{
  int xreg, yreg, zreg;
  bool ok = getXYZRawValues(xreg,yreg,zreg);
  double lsbgain=GAIN[fGain];
  x=lsbgain*xreg/1000.;
  y=lsbgain*yreg/1000.;
  z=lsbgain*zreg/1000.;
  
  if (fDebugLevel>1)
  {
    printf( "x field: %f G\n",x);
    printf( "y field: %f G\n",y);
    printf( "z field: %f G\n",z);
  }  
  
  return ok;
}

bool HMC5883::calibrate(int &x, int &y, int &z)
{
  
  // addr config reg A (CRA)
  // 8 average, 15 Hz, positive self test measurement: 0x71
  uint8_t cmd = 0x71;
  /*int n=*/writeReg(0x00, &cmd, 1);
  
  uint8_t oldGain=fGain;
  setGain(5);
  
  int xr, yr, zr;
  // one dummy measurement
  getXYZRawValues(xr,yr,zr);
  // measurement
  getXYZRawValues(xr,yr,zr);
  
  x=xr; y=yr; z=zr;
  
  setGain(oldGain);
  // one dummy measurement
  getXYZRawValues(xr,yr,zr);
  

  // set normal measurement mode in CRA again
  cmd = 0x70;
  /*n=*/writeReg(0x00, &cmd, 1);
  return true;
}
