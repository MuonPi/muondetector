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
	: LM75(0x4f)
{
	fTitle = "MIC184";
}

MIC184::MIC184(const char* busAddress, uint8_t slaveAddress)
	: LM75(busAddress, slaveAddress)
{
	fTitle = "MIC184";
}

MIC184::MIC184(uint8_t slaveAddress)
	: LM75(slaveAddress)
{
	fTitle = "MIC184";
}

MIC184::~MIC184()
{
}

bool MIC184::identify()
{
	if ( fMode == MODE_FAILED ) return false;
	if ( !devicePresent() ) return false;

	bool ok = LM75::identify();
	if ( !ok ) return false;
	
	uint8_t conf_reg_save { 0 };
	uint16_t thyst_save { 0 };
	uint16_t tos_save { 0 };
	
	// Read and save the config register
	if ( !readByte( static_cast<uint8_t>(REG::CONF), &conf_reg_save ) ) return false;
	// read and save the Thyst register
	if ( !readWord( static_cast<uint8_t>(REG::THYST), &thyst_save ) ) return false;
	// read and save the Tos register
	if ( !readWord( static_cast<uint8_t>(REG::TOS), &tos_save ) ) return false;
	
	// determine, whether we have a MIC84 or just a plain LM75
	// datasheet: test, if the STS (status) bit in config register toggles when a alarm condition is provoked
	// set config reg to 0x02
	uint8_t conf_reg { 0x02 };
	if ( !writeByte( static_cast<uint8_t>(REG::CONF), conf_reg ) ) return false;
	// write 0xc880 to Thyst and Tos regs. This corresponds to -55.5 degrees centigrade
	uint16_t dataword { 0xc880 };
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
	// Read the config register
	if ( !readByte( static_cast<uint8_t>(REG::CONF), &conf_reg ) ) return false;
	// datasheet: STS bit (MSB of conf reg) should be reset now
	// this is considered an indication for MIC184
	if ( ( conf_reg & 0x80 ) ) return false;
	
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

