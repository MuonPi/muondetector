#include "i2cdevices.h"
#include <iostream>

extern "C"{
	// to make g++ know that custom_i2cdetect is written in c and not cpp (makes the linking possible)
	// You may want to check out on this here: (last visit 29. of May 2017)
	// https://stackoverflow.com/questions/1041866/in-c-source-what-is-the-effect-of-extern-c
	#include "custom_i2cdetect.h"
}


int main() {
	if (!i2cdetect == 0) {
	}
	LM75 tempSensor("/dev/i2c-1",0x4f);
	double temperature = tempSensor.getTemperature();
	std::cout << "Temperatur: " << temperature << std::endl;
	ADS1015 adc("/dev/i2c-1", 0x48);
	double adcValue = adc.readVoltage(0);
	std::cout << "ADC Value: " << adcValue << std::endl;
	MCP4728 dac("/dev/i2c-1", 0x60);
	dac.setVoltage(0, 4.35);
	PCA9536 pca("/dev/i2c-1", 0x41);
	pca.setOutputPorts(0b1101);
}