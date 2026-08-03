#include "i2cdevices.h"

#include <iomanip>
#include <iostream>

i2cDevice* instantiateI2cDevice(uint8_t addr) {
    i2cDevice* device{nullptr};
    for (uint8_t i = 0; i < i2cDevice::getGlobalDeviceList().size(); i++) {
        if (addr == i2cDevice::getGlobalDeviceList()[i]->getAddress()) {
            return device;
        }
    }

    bool ident{false};
    ident = MCP4728::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new MCP4728(addr);
        device->identify();
        return device;
    }
    ident = MIC184::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new MIC184(addr);
        device->identify();
        return device;
    }
    ident = LM75::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new LM75(addr);
        device->identify();
        return device;
    }
    ident = ADS1115::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new ADS1115(addr);
        device->identify();
        return device;
    }
    ident = EEPROM24AA02::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new EEPROM24AA02(addr);
        device->identify();
        return device;
    }
    ident = PCA9536::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new PCA9536(addr);
        device->identify();
        return device;
    }
    ident = HMC5883::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new HMC5883(addr);
        device->identify();
        return device;
    }
    ident = QMC5883::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new QMC5883(addr);
        device->identify();
        return device;
    }
    ident = VEML6075::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new VEML6075(addr);
        device->identify();
        return device;
    }
    ident = MPU6050::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new MPU6050(addr);
        device->identify();
        return device;
    }
    ident = SEN0321::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new SEN0321(addr);
        device->identify();
        return device;
    }
    ident = Ozone3Click::identifyDevice(static_cast<uint8_t>(addr));
    if (ident) {
        device = new Ozone3Click(addr);
        device->identify();
        return device;
    }

    device = new i2cDevice(addr);
    return device;
}
