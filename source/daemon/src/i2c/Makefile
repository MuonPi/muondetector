#	This makefile uses gcc (c11) to compile all .c-files and g++ (c++0x) to compile all .cpp-files
#	information about how to write makefiles: https://www.cs.umd.edu/class/fall2002/cmsc214/Tutorial/makefile.html
#	(29. of May 2017)
#	tested with g++ and gcc version: Raspbian 4.9.2-10
#
OBJS = custom_i2cdetect.o i2cbusses.o i2cdevices.o
C = gcc -std=c11
CPP = g++ -std=c++11
CFLAGS = -c -O
	
all: custom_i2cdetect.o i2cbusses.o i2cdevices.o
	
custom_i2cdetect.o : custom_i2cdetect.c custom_i2cdetect.h addresses.h linux/i2c-dev.h i2cbusses.h
	$(C) $(CFLAGS) custom_i2cdetect.c

i2cbusses.o : i2cbusses.c i2cbusses.h linux/i2c-dev.h
	$(C) $(CFLAGS) i2cbusses.c

i2cdevices.o : i2cdevices.cpp i2cdevices.h 
	$(CPP) $(CFLAGS) i2cdevices.cpp

clean :	
	\rm -f *.o *~ 
