#ifndef _PCA9536_H_
#define _PCA9536_H_
#include "hardware/device_types.h"
#include "hardware/i2c/i2cdevice.h"

/* PCA9536  */
class PCA9536 : public i2cDevice, public DeviceFunction<DeviceType::IO_EXTENDER>, public static_device_base<PCA9536> {
    // the device supports reading the incoming logic levels of the pins if set to input in the configuration register (will probably not use this feature)
    // the device supports polarity inversion (by configuring the polarity inversion register) (will probably not use this feature)
public:
    enum CFG_PORT { C0 = 0,
        C1 = 2,
        C3 = 4,
        C4 = 8 };
    PCA9536()
        : i2cDevice(0x41)
    {
        fTitle = fName = "PCA9536";
    }
    PCA9536(const char* busAddress, uint8_t slaveAddress)
        : i2cDevice(busAddress, slaveAddress)
    {
        fTitle = fName = "PCA9536";
    }
    PCA9536(uint8_t slaveAddress)
        : i2cDevice(slaveAddress)
    {
        fTitle = fName = "PCA9536";
    }
    bool setOutputPorts(uint8_t portMask) override;
    bool setOutputState(uint8_t portMask) override;
    uint8_t getInputState() override;
    bool devicePresent() override;
    bool identify() override;
    bool probeDevicePresence() override { return devicePresent(); }

private:
    enum REG {
        INPUT = 0x00,
        OUTPUT = 0x01,
        POLARITY = 0x02,
        CONFIG = 0x03
    };
};

#endif // !_PCA9536_H_
