#include "hardware/i2c/i2cutil.h"
#include <iomanip>
#include <iostream>

I2cGeneralCall::I2cGeneralCall()
    : i2cDevice(static_cast<uint8_t>(0x00))
{
    fTitle = "GeneralCall";
}

I2cGeneralCall::I2cGeneralCall(const char* busAddress)
    : i2cDevice(busAddress, static_cast<uint8_t>(0x00))
{
    fTitle = "GeneralCall";
}

void I2cGeneralCall::resetDevices()
{
    I2cGeneralCall gc;
    uint8_t data { 0x06 };
    gc.write(&data, 1);
}

void I2cGeneralCall::wakeUp()
{
    I2cGeneralCall gc;
    uint8_t data { 0x09 };
    gc.write(&data, 1);
}

void I2cGeneralCall::softwareUpdate()
{
    I2cGeneralCall gc;
    uint8_t data { 0x08 };
    gc.write(&data, 1);
}
