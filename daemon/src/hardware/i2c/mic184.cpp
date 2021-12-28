#include "hardware/i2c/mic184.h"
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
/*
* MIC184 Temperature Sensor
*/
MIC184::MIC184()
	: i2cDevice(0x4f)
{
	fTitle = "MIC184";
}

MIC184::MIC184(const char* busAddress, uint8_t slaveAddress)
	: i2cDevice(busAddress, slaveAddress)
{
	fTitle = "MIC184";
}

MIC184::MIC184(uint8_t slaveAddress)
	: i2cDevice(slaveAddress)
{
	fTitle = "MIC184";
}

MIC184::~MIC184()
{
}

int16_t MIC184::readRaw()
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

bool MIC184::devicePresent()
{
    uint8_t readBuf[2]; // 2 byte buffer to store the data read from the I2C device
    readBuf[0] = 0;
    readBuf[1] = 0;
    int n = read(readBuf, 2);
    return (n == 2);
}

float MIC184::getTemperature()
{
	int16_t dataword = readRaw();
	float temp = static_cast<float>( dataword >> 8 );
	temp += static_cast<float>(dataword & 0xff)/256.; 
	fLastTemp = temp;
    return temp;
}

bool MIC184::identify()
{
	if ( fMode == MODE_FAILED ) return false;
	if ( !devicePresent() ) return false;

	uint8_t conf_reg_save { 0 };
	uint16_t dataword { 0 };
	uint16_t thyst_save { 0 };
	uint16_t tos_save { 0 };

	// Read the config register
	if ( !readByte( static_cast<uint8_t>(REG::CONF), &conf_reg_save ) ) {
		// there was an error
		return false;
	}
	// datasheet: 3 MSBs of conf register should be all zero when device is in init state
	if ( ( conf_reg_save >> 5 ) != 0 ) return false;
	
	// read temp register
	if ( !readWord( static_cast<uint8_t>(REG::TEMP), &dataword ) ) {
		// there was an error
		return false;
	}
	// the 5 LSBs should always read zero
	if ( (dataword & 0x1f) != 0 ) return false;
//	if ( ( (dataword & 0x1f) != 0 ) && ( dataword >> 5 ) == 0 ) return false;
	
	// read Thyst register
	if ( !readWord( static_cast<uint8_t>(REG::THYST), &thyst_save ) ) {
		// there was an error
		return false;
	}
	// the 7 MSBs should always read zero
	if ( (thyst_save & 0x7f) != 0 ) return false;

	// read Tos register
	if ( !readWord( static_cast<uint8_t>(REG::TOS), &tos_save ) ) {
		// there was an error
		return false;
	}
	// the 7 MSBs should always read zero
	if ( (tos_save & 0x7f) != 0 ) return false;
/*
	std::cout << "MIC184::identify() : found LM75 base device at 0x"<<std::setw(2) << std::setfill('0')<<std::hex<<(int)fAddress<<"\n"; 
	std::cout << " Regs: \n";
	std::cout << "  conf  = 0x"<<std::setw(2) << std::setfill('0')<<(int)conf_reg_save<<"\n";
	std::cout << "  thyst = 0x"<<std::setw(4) << std::setfill('0')<<thyst_save<<"\n";
	std::cout << "  tos   = 0x"<<std::setw(4) << std::setfill('0')<<tos_save<<"\n";
	std::cout << "  temp  = 0x"<<std::setw(4) << std::setfill('0')<<dataword<<"\n";
*/	
	
	// determine, whether we have a MIC184 or just a plain LM75
	// datasheet: test, if the STS (status) bit in config register toggles when a alarm condition is provoked
	// set config reg to 0x02
	uint8_t conf_reg { 0x02 };
	if ( !writeByte( static_cast<uint8_t>(REG::CONF), conf_reg ) ) return false;
	// write 0xc880 to Thyst and Tos regs. This corresponds to -55.5 degrees centigrade
	dataword = 0xc880;
	if ( !writeWord( static_cast<uint8_t>(REG::THYST), dataword ) ) return false;
	if ( !writeWord( static_cast<uint8_t>(REG::TOS), dataword ) ) return false;
	// wait at least one conversion cycle (>160ms)
	std::this_thread::sleep_for( std::chrono::milliseconds( 160 ) );
	// Read the config register
	if ( !readByte( static_cast<uint8_t>(REG::CONF), &conf_reg ) ) return false;
	// datasheet: MSB of conf reg should be set to one
	// this is considered an indication for MIC184
	if ( !( conf_reg & 0x80 ) ) return false;
	
	// write 0x7f80 to Thyst and Tos regs. This corresponds to +127.5 degrees centigrade
	dataword = 0x7f80;
	if ( !writeWord( static_cast<uint8_t>(REG::THYST), dataword ) ) return false;
	if ( !writeWord( static_cast<uint8_t>(REG::TOS), dataword ) ) return false;
	// wait at least one conversion cycle (>160ms)
	std::this_thread::sleep_for( std::chrono::milliseconds( 160 ) );
	// Read the config register again to clear pending interrupt request
	if ( !readByte( static_cast<uint8_t>(REG::CONF), &conf_reg ) ) return false;
	
	// at this point we know for sure that the device is an MIC184
	// set THyst and Tos regs back to previous settings
	writeWord( static_cast<uint8_t>(REG::THYST), thyst_save );
	writeWord( static_cast<uint8_t>(REG::TOS), tos_save );
	// finally, set config reg into original state
	if ( writeByte( static_cast<uint8_t>(REG::CONF), conf_reg_save ) ) {
		fExternal = ( conf_reg_save & 0x20 );
		return true;
	}
	return false;
}

bool MIC184::setExternal( bool enable_external )
{
	// Read and save the config register
	uint8_t conf_reg { 0 };
	uint8_t conf_reg_save { 0 };
	if ( !readByte( static_cast<uint8_t>(REG::CONF), &conf_reg ) ) return false;
	conf_reg_save = conf_reg;
	// disable interrupts, clear IM bit
	conf_reg &= ~0x40;
	if ( !writeByte( static_cast<uint8_t>(REG::CONF), conf_reg ) ) return false;
	// read back config reg to clear STS flag
	if ( !readByte( static_cast<uint8_t>(REG::CONF), &conf_reg ) ) return false;
	if ( enable_external ) conf_reg_save |= 0x20;
	else conf_reg_save &= ~0x20;
	if ( !writeByte( static_cast<uint8_t>(REG::CONF), conf_reg_save ) ) return false;
	if ( !readByte( static_cast<uint8_t>(REG::CONF), &conf_reg ) ) return false;
	if ( (conf_reg & 0x20) != (conf_reg_save & 0x20) ) return false;
	fExternal = enable_external;
	return true;
}
