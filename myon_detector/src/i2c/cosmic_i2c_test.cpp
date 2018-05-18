#include "i2cdevices.h"
#include <iostream>
#include <stdio.h>
using namespace std;

extern "C"{
	// to make g++ know that custom_i2cdetect is written in c and not cpp (makes the linking possible)
	// You may want to check out on this here: (last visit 29. of May 2017)
	// https://stackoverflow.com/questions/1041866/in-c-source-what-is-the-effect-of-extern-c
	#include "custom_i2cdetect.h"
}

int main() {
	if (!i2cdetect == 0) {
	}
	/*
	LM75 tempSensor("/dev/i2c-1",0x4f);
	double temperature = tempSensor.getTemperature();
	std::cout << "Temperatur: " << temperature << std::endl;
	ADS1015 adc("/dev/i2c-1", 0x48);
	double adcValue = adc.readVoltage(0);
	std::cout << "ADC Value: " << adcValue << std::endl;*/
	PCA9536 pca("/dev/i2c-1", 0x41);
	pca.setOutputPorts(3); 
	pca.setOutputState(0); // sets TIME_SEL0 = 0, TIME_SEL1 = 1 -> 74AC151SJ forwards channel DISCR1.1 to ublox TIME_MEAS
	MCP4728 dac("/dev/i2c-1", 0x60);
	while (true) {
		int channel = 0;
		cout << "Set channel:" << endl;
		cin >> channel;
		if (channel > 3 || channel < 0) {
			continue;
		}
		cout << endl << "Set voltage:" << endl;
		float voltage = 0;
		cin >> voltage;
		cout << endl;
		if (voltage < 0) {
			continue;
		}
		dac.setVoltage((uint8_t)channel, voltage);
	}
	//PCA9536 pca("/dev/i2c-1", 0x41);
	//pca.setOutputPorts(0b1101);
}