#ifndef _I2CUTIL_H_
#define _I2CUTIL_H_

#include "hardware/i2c/i2cdevice.h"

class I2cGeneralCall : private i2cDevice {
public:
	static void resetDevices()
	{
		I2cGeneralCall gc;
		uint8_t data { 0x06 };
		gc.write( &data, 1 );
	}
private:
	I2cGeneralCall()
	: i2cDevice( static_cast<uint8_t>( 0x00 ) )
	{
		fTitle = "GeneralCall";
	}
	I2cGeneralCall(const char* busAddress)
	: i2cDevice( busAddress, static_cast<uint8_t>( 0x00 ) )
	{
		fTitle = "GeneralCall";
	}
};

#endif // !_I2CUTIL_H_
