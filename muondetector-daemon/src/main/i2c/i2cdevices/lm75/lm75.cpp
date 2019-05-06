#include "lm75.h"
#include <stdint.h>

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
	val = ((int16_t)readBuf[0] << 8) | readBuf[1];
	fLastRawValue = val;

	stopTimer();

	return val;
}

bool LM75::devicePresent()
{
	uint8_t readBuf[2];		// 2 byte buffer to store the data read from the I2C device  
	readBuf[0] = 0;
	readBuf[1] = 0;
	int n = read(readBuf, 2);	// Read the data register into readBuf
	return (n == 2);
}

double LM75::getTemperature()
{
	return (double)readRaw() / 256.;
}

