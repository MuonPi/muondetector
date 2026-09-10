#ifndef DEVICES_H
#define DEVICES_H

#include "device_types.h"

#include <cstdint>
#include <map>

enum class Device : std::uint32_t {
    ADS1115_0,
    MCP4728_0,
    PCA9536_0,
    EEPROM24AA02_0,
    LM75_0,
    MIC184_0,
    MPU6050_0,
    HMC5883_0,
    QMC5883_0,
    VEML6075_0,
    SEN0321_0,
    OZONE3CLICK_0,
    UBLOX_I2C_0,
    ADAFRUIT_SSD1306_0,
    ADXL355_0
};

inline const std::map<std::string, Device> deviceLookup = {
    {"ADS1115_0", Device::ADS1115_0},
    {"MCP4728_0", Device::MCP4728_0},
    {"PCA9536_0", Device::PCA9536_0},
    {"EEPROM24AA02_0", Device::EEPROM24AA02_0},
    {"LM75_0", Device::LM75_0},
    {"MIC184_0", Device::MIC184_0},
    {"MPU6050_0", Device::MPU6050_0},
    {"GY521_0", Device::MPU6050_0},
    {"HMC5883_0", Device::HMC5883_0},
    {"QMC5883_0", Device::QMC5883_0},
    {"VEML6075_0", Device::VEML6075_0},
    {"SEN0321_0", Device::SEN0321_0},
    {"OZONE3CLICK_0", Device::OZONE3CLICK_0},
    {"UBLOX_I2C_0", Device::UBLOX_I2C_0},
    {"ADAFRUIT_SSD1306_0", Device::ADAFRUIT_SSD1306_0},
    {"ADXL355_0", Device::ADXL355_0}
};

#endif // DEVICES_H