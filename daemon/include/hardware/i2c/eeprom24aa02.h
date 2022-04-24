#ifndef _EEPROM24AA02_H_
#define _EEPROM24AA02_H_

#include "hardware/device_types.h"
#include "hardware/i2c/i2cdevice.h"

/* EEPROM24AA02  */

class EEPROM24AA02 : public i2cDevice, public DeviceFunction<DeviceType::EEPROM>, public static_device_base<EEPROM24AA02> {
public:
    EEPROM24AA02()
        : i2cDevice(0x50)
    {
        fTitle = fName = "24AA02";
    }
    EEPROM24AA02(const char* busAddress, uint8_t slaveAddress)
        : i2cDevice(busAddress, slaveAddress)
    {
        fTitle = fName = "24AA02";
    }
    EEPROM24AA02(uint8_t slaveAddress)
        : i2cDevice(slaveAddress)
    {
        fTitle = fName = "24AA02";
    }

    uint8_t readByte(uint8_t addr);
    bool readByte(uint8_t addr, uint8_t* value);
    void writeByte(uint8_t addr, uint8_t data);
    /** Write multiple bytes to starting from given address into EEPROM memory.
	* @param addr First register address to write to
	* @param length Number of bytes to write
	* @param data Buffer to copy new data from
	* @return Status of operation (true = success)
	* @note this is an overloaded function to the one from the i2cDevice base class in order to
	* prevent sequential write operations crossing page boundaries of the EEPROM. This function conforms to
	* the page-wise sequential write (c.f. http://ww1.microchip.com/downloads/en/devicedoc/21709c.pdf  p.7).
	*/
    bool writeBytes(uint8_t addr, uint16_t length, uint8_t* data) override;
    int16_t readBytes(uint8_t regAddr, uint16_t length, uint8_t* data) override;

    bool identify() override;
    bool probeDevicePresence() override { return devicePresent(); }

private:
    // hide all write functions from i2cDevice class since they do not conform to the correct write sequence of the eeprom
    using i2cDevice::readBytes;
    using i2cDevice::write;
    using i2cDevice::writeByte;
    using i2cDevice::writeBytes;
    using i2cDevice::writeWord;
    using i2cDevice::writeWords;
};
#endif //!_EEPROM24AA02_H
