#ifndef _LM75_H_
#define _LM75_H_
#include "hardware/i2c/i2cdevice.h"

class LM75 : public i2cDevice, public static_device_base<LM75> {
public:
    LM75()
        : i2cDevice(0x4f)
    {
        fTitle = "LM75A";
    }
    LM75(const char* busAddress, uint8_t slaveAddress)
        : i2cDevice(busAddress, slaveAddress)
    {
        fTitle = "LM75A";
    }
    LM75(uint8_t slaveAddress)
        : i2cDevice(slaveAddress)
    {
        fTitle = "LM75A";
    }
    bool devicePresent();
    int16_t readRaw();
    float getTemperature();
    float lastTemperatureValue() const { return fLastTemp; }

    bool identify() override;
private:
	enum REG : uint8_t {
		TEMP = 0x00,
		CONF = 0x01,
		THYST = 0x02,
		TOS = 0x03
	};

	unsigned int fLastConvTime;
    signed int fLastRawValue;
    float fLastTemp;
};
#endif // !_LM75_H_
