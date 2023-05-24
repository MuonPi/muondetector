#ifndef _LM75_H_
#define _LM75_H_
#include "hardware/device_types.h"
#include "hardware/i2c/i2cdevice.h"

class LM75 : public i2cDevice, public DeviceFunction<DeviceType::TEMP>, public static_device_base<LM75> {
public:
    LM75();
    LM75(const char* busAddress, uint8_t slaveAddress);
    LM75(uint8_t slaveAddress);
    virtual ~LM75();
    float getTemperature() override;

    bool identify() override;
    bool probeDevicePresence() override { return devicePresent(); }

protected:
    int16_t readRaw();

    enum REG : uint8_t {
        TEMP = 0x00,
        CONF = 0x01,
        THYST = 0x02,
        TOS = 0x03
    };

    unsigned int fLastConvTime;
    signed int fLastRawValue;
};
#endif // !_LM75_H_
