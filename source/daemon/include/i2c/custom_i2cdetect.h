/*
custom_i2cdetect is directly copied from i2c-tools-3.1.2 i2cdetect.c and edited
it now contains "i2cdetect()" as a function which does the same as main() did before except a few changes.
Now it can be called by the main program and it's the only purpose of custom_i2cdetect right now.
*/
#ifndef _CUSTOM_I2CDETECT_H
#define _CUSTOM_I2CDETECT_H

#include "i2c/addresses.h"
#include "i2c/i2cbusses.h"

#include <errno.h>
#include <linux/i2c-dev.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MODE_AUTO 0
#define I2C_BUS 1

int scan_i2c_bus(int file, int first, int last,
    bool outputAllAddresses, int expectedAddresses[]);
int i2cdetect(bool outputAllAddresses, int expectedAddresses[]);
#endif
