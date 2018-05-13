#include "i2cdevices.h"
#include <iostream>

int main() {
	LM75 tempSensor("/dev/i2c-1",0x4f);
	double temperature = tempSensor.getTemperature();
	std::cout << "Temperatur: " << temperature << std::endl;
	ADS1015 adc("/dev/i2c-1", 0x48);
	double adcValue = adc.readVoltage(0);
	std::cout << "ADC Value: " << adcValue << std::endl;
}