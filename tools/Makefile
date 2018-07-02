#	This makefile uses gcc (c11) to compile all .c-files and g++ (c++0x) to compile all .cpp-files
#	information about how to write makefiles: https://www.cs.umd.edu/class/fall2002/cmsc214/Tutorial/makefile.html
#	(29. of May 2017)
#	tested with g++ and gcc version: Raspbian 4.9.2-10
#
SHARED_PATH = ../shared/i2c/
OBJS = $(SHARED_PATH)custom_i2cdetect.o $(SHARED_PATH)i2cbusses.o $(SHARED_PATH)i2cdevices.o
C = gcc -std=c11
CPP = g++ -std=c++11
LFLAGS = -O -lwiringPi -lpigpio
CFLAGS = -c -O
	
all: shared cosmic_i2c_test cosmic_gpio_ratecounter
	
cosmic_gpio_ratecounter : $(OBJS) cosmic_gpio_ratecounter.o
	$(CPP) $(OBJS) cosmic_gpio_ratecounter.o $(LFLAGS) -o cosmic_gpio_ratecounter

cosmic_i2c_test : $(OBJS) cosmic_i2c_test.o
	$(CPP) $(OBJS) cosmic_i2c_test.o $(LFLAGS) -o cosmic_i2c_test


shared : 
	$(MAKE) -C $(SHARED_PATH)

clean :	
	\rm -f *.o *~ cosmic_i2c_test cosmic_gpio_ratecounter

