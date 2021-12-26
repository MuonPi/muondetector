#include "hardware/i2c/i2cutil.h"
#include <iostream>
#include <iomanip>

I2cGeneralCall::I2cGeneralCall()
: i2cDevice( static_cast<uint8_t>( 0x00 ) )
{
	fTitle = "GeneralCall";
}

I2cGeneralCall::I2cGeneralCall(const char* busAddress)
: i2cDevice( busAddress, static_cast<uint8_t>( 0x00 ) )
{
	fTitle = "GeneralCall";
}

void I2cGeneralCall::resetDevices()
{
	I2cGeneralCall gc;
	uint8_t data { 0x06 };
	gc.write( &data, 1 );
}
