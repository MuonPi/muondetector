/*
Test of HV Distribution Board I2C Devices
compile with: make
or:
g++ -std=c++0x -O -c i2cdevices.cpp && g++ -std=c++0x -O -c board-check-and-distro.cpp
&& gcc -std=c11 -O -c custom_i2cdetect.c && gcc -std=c11 -O -c i2cbusses.c
&& g++ -std=c++0x -O board-check-and-distro.o i2cdevices.o custom_i2cdetect.o i2cbusses.o -o board-check-and-distro
*/
#include <stdlib.h>

#include <stdio.h>
#include <fcntl.h>     // open
#include <inttypes.h>  // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions
#include <ctime>
#include <iostream>
#include <algorithm>
#include <deque>
#include "i2cdevices.h"

#define NUMBER_OF_CHANNELS 4

extern "C" {
	// to make g++ know that custom_i2cdetect is written in c and not cpp (makes the linking possible)
	// You may want to check out on this here: (last visit 29. of May 2017)
	// https://stackoverflow.com/questions/1041866/in-c-source-what-is-the-effect-of-extern-c
#include "custom_i2cdetect.h"
}
//#include "data_analysis_tools.h"
#include "addresses.h"

class Stats {
public:
	std::deque<double> buf;
	Stats() : maxBufSize(100) {}
	Stats(unsigned int bufSize) : maxBufSize(bufSize) {}
	inline void add(double& val) {
		buf.push_back(val);
		if (buf.size()>maxBufSize) buf.pop_front();
	}
	inline unsigned int nrEntries() { return buf.size(); }
	double getMean();
	inline double getRMS();
	unsigned int maxBufSize;
};