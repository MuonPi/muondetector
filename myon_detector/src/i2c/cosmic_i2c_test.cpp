#include <wiringPi.h>
#include "i2cdevices.h"
#include <iostream>
#include <stdio.h>
using namespace std;

#define UBIAS_EN 7
#define PREAMP_1 28
#define PREAMP_2 29

extern "C" {
	// to make g++ know that custom_i2cdetect is written in c and not cpp (makes the linking possible)
	// You may want to check out on this here: (last visit 29. of May 2017)
	// https://stackoverflow.com/questions/1041866/in-c-source-what-is-the-effect-of-extern-c
#include "custom_i2cdetect.h"
}

void showInstructions() {
	cout << "input:\n'lm75' or 't' to show temperature\n'adc' or 'v' to show voltage\n'pca' to start configuring ublox measuring channel" << endl;
	cout << "'dac' to start configuring thresholds\n'p' or 'power'\n'i2cdetect' to show i2cdetect -y 1 output\n'h' to show help" << endl;
}

int main() {
	wiringPiSetup();
	pinMode(UBIAS_EN, 1);
	digitalWrite(UBIAS_EN, 0);
	pinMode(PREAMP_1, 1);
	digitalWrite(PREAMP_1, 1);
	pinMode(PREAMP_2, 1);
	digitalWrite(PREAMP_2, 1);
	bool powerOn = false;


	/*if (i2cdetect()!=0) {
		cerr << "i2cdetect failed" << endl;
	}
	*/

	LM75 tempSensor("/dev/i2c-1", 0x4f);
	ADS1015 adc("/dev/i2c-1", 0x48);
	PCA9536 pca("/dev/i2c-1", 0x41);
	pca.setOutputPorts(3);
	pca.setOutputState(0);
	MCP4728 dac("/dev/i2c-1", 0x60);
	showInstructions();
	while (true) {
		string input = "";
		cin >> input;
		if (input == "power" || input == "p") {
			if (powerOn) {
				digitalWrite(UBIAS_EN, 0);
				powerOn = false;
				cout << "turning off HV" << endl;
			}
			else {
				digitalWrite(UBIAS_EN, 1);
				powerOn = true;
				cout << "turning on HV" << endl;
			}
		}
		if (input == "lm75" || input == "t") {
			double temperature = tempSensor.getTemperature();
			std::cout << "Temperatur: " << temperature << std::endl;
		}
		if (input == "adc" || input == "v") {
			//double adcValues[4];
			for (int i = 0; i < 4; i++) {
				//adcValues[i] = adc.readVoltage(i);
				double voltage = adc.readVoltage(i);
				cout << "ADC Channel " << i << ": " << voltage << "V";
			}
			cout << endl;
		}
		if (input == "pca") {
			cout << "select portMask:" << endl;
			float channel = 0;
			cin >> channel;
			if ((int)channel >= 0 && (int)channel < 4) {
				pca.setOutputState((int)channel); // sets TIME_SEL0 = 0, TIME_SEL1 = 1 -> 74AC151SJ forwards channel DISCR1.1 to ublox TIME_MEAS
			}
			cout << endl;
		}
		if (input == "dac") {
			double voltage = 0;
			double channel = 0;
			cout << "Set channel:" << endl;
			cin >> channel;
			if ((int)channel < 4 && (int)channel >= 0) {
				cout << "Set voltage:" << endl;
				double voltage = 0;
				cin >> voltage;
				if (voltage >= 0) {
					cout << "set voltage to " << voltage << endl;
					dac.setVoltage((uint8_t)channel, voltage);
				}
			}
		}
		if (input == "h") {
			showInstructions();
		}
		if (input == "i2cdetect") {
			int newInt = i2cdetect();
		}
	}
}