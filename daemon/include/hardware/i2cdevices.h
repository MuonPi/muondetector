#ifndef _I2CDEVICES_H_
#define _I2CDEVICES_H_

#include "hardware/i2c/adafruit_ssd1306.h"
#include "hardware/i2c/ads1015.h"
#include "hardware/i2c/ads1115.h"
#include "hardware/i2c/bme280.h"
#include "hardware/i2c/bmp180.h"
#include "hardware/i2c/eeprom24aa02.h"
#include "hardware/i2c/hmc5883.h"
#include "hardware/i2c/lm75.h"
#include "hardware/i2c/mcp4728.h"
#include "hardware/i2c/mic184.h"
#include "hardware/i2c/pca9536.h"
#include "hardware/i2c/sht21.h"
#include "hardware/i2c/sht31.h"
#include "hardware/i2c/tca9546a.h"
#include "hardware/i2c/ubloxi2c.h"
#include "hardware/i2c/x9119.h"

//#include "hardware/i2c/i2cdevice.h"

class i2cDevice;

i2cDevice* instantiateI2cDevice(uint8_t addr);

#endif // !_I2CDEVICES_H_
