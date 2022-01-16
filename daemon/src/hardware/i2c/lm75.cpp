#include "hardware/i2c/lm75.h"
#include <stdint.h>
#include <iostream>
#include <iomanip>
/*
* LM75 Temperature Sensor
*/
LM75::LM75()
	: i2cDevice(0x4f)
{
	fTitle = fName = "LM75";
}

LM75::LM75(const char* busAddress, uint8_t slaveAddress)
	: i2cDevice(busAddress, slaveAddress)
{
	fTitle = fName = "LM75";
}

LM75::LM75(uint8_t slaveAddress)
	: i2cDevice(slaveAddress)
{
	fTitle = fName = "LM75";
}

LM75::~LM75()
{
	
}

int16_t LM75::readRaw()
{
    startTimer();

    uint16_t dataword { 0 };
	// Read the temp register
	if ( !readWord( static_cast<uint8_t>(REG::TEMP), &dataword ) ) {
		// there was an error
		return INT16_MIN;
	}

	int16_t val = static_cast<int16_t>( dataword );
//    val = ((int16_t)readBuf[0] << 8) | readBuf[1];
    fLastRawValue = val;

    stopTimer();

    return val;
}

float LM75::getTemperature()
{
	int16_t dataword = readRaw();
	float temp = static_cast<float>( dataword >> 8 );
	temp += static_cast<float>(dataword & 0xff)/256.; 
	fLastTemp = temp;
    return temp;
}

bool LM75::identify()
{
	if ( fMode == MODE_FAILED ) return false;
	if ( !devicePresent() ) return false;
    uint16_t dataword { 0 };
	uint8_t conf_reg { 0 };
	// Read the config register
	if ( !readByte( static_cast<uint8_t>(REG::CONF), &conf_reg ) ) {
		// there was an error
		return false;
	}
	// datasheet: 3 MSBs of conf register "should be kept as zeroes"
	if ( ( conf_reg >> 5 ) != 0 ) return false;
	
	// read temp register
	if ( !readWord( static_cast<uint8_t>(REG::TEMP), &dataword ) ) {
		// there was an error
		return false;
	}
	// the 5 LSBs should always read zero
	if ( (dataword & 0x1f) != 0 ) return false;
//	if ( ( (dataword & 0x1f) != 0 ) && ( dataword >> 5 ) == 0 ) return false;
	
	// read Thyst register
	if ( !readWord( static_cast<uint8_t>(REG::THYST), &dataword ) ) {
		// there was an error
		return false;
	}
	// the 7 MSBs should always read zero
	if ( (dataword & 0x7f) != 0 ) return false;

	// read Tos register
	if ( !readWord( static_cast<uint8_t>(REG::TOS), &dataword ) ) {
		// there was an error
		return false;
	}
	// the 7 MSBs should always read zero
	if ( (dataword & 0x7f) != 0 ) return false;
	
	return true;
}
