#ifndef _MIC184_H_
#define _MIC184_H_
#include "hardware/i2c/i2cdevice.h"
//#include "hardware/i2c/lm75.h"
#include "hardware/device_types.h"

class MIC184 : public i2cDevice, public DeviceFunction<DeviceType::TEMP>, public static_device_base<MIC184> {
public:
    MIC184();
    MIC184(const char* busAddress, uint8_t slaveAddress);
    MIC184(uint8_t slaveAddress);
	virtual ~MIC184();
    bool devicePresent();
    int16_t readRaw();
    float getTemperature() override;
        
	bool identify() override;
	bool isExternal() const { return fExternal; }
	bool setExternal( bool enable_external = true );
private:
	enum REG : uint8_t {
		TEMP = 0x00,
		CONF = 0x01,
		THYST = 0x02,
		TOS = 0x03
	};

	unsigned int fLastConvTime;
    signed int fLastRawValue;
	bool fExternal { false };
};
#endif // !_MIC184_H_
