/*
custom_i2cdetect is directly copied from i2c-tools-3.1.2 i2cdetect.c and edited
it now contains "i2cdetect()" as a function which does the same as main() did before except a few changes.
Now it can be called by the main program and it's the only purpose of custom_i2cdetect right now.
*/
#ifndef _CUSTOM_I2CDETECT_H
#define _CUSTOM_I2CDETECT_H

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <i2cbusses.h>
#include <addresses.h>

#define MODE_AUTO 0
#define I2C_BUS 1

int scan_i2c_bus(int file, /*int mode,*/ int first, int last,
	bool outputAllAddresses, int expectedAddresses[]);
/*
struct func
{
	long value;
	const char* name;
};
static const struct func all_func[];
*/
//static void print_functionality(unsigned long funcs);
// int i2cdetect();
// int i2cdetect(int expectedAddresses[]);
// c does not like overloading those functions
int i2cdetect(bool outputAllAddresses, int expectedAddresses[]);
#endif
