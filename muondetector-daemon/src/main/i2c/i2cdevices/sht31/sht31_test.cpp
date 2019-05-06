#include "sht31.h"
#include <unistd.h>
#include <iostream>

using namespace std;

int main() {
	SHT31 sensor = SHT31();
	uint16_t UT = 0;
	uint16_t UH = 0;
	float temp = 0., hum = 0.;
	sensor.softReset();
	sensor.breakCommand();
	for (int j = 0; j < 3; j++) {
		sensor.heater(j % 2);
		for (int i = 0; i < 5; i++) {
			cout << sensor.readRaw(UT, UH) << " " << UT << " " << UH << "   ";
			cout << sensor.getValues(temp, hum) << " " << temp << " " << hum << "   heater: " << j%2 << endl;
			usleep(2000000);
		}
	}
	return 0;
}