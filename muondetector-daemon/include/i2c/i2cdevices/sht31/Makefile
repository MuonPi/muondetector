LFLAGS = -O
CPP = g++ -std=c++11
CFLAGS = -c -O
OBJS = sht31.o i2cdevice.o sht31_test.o
		
sht31_test : $(OBJS)
	$(CPP) $(OBJS) $(LFLAGS) -o sht31_test
	
sht31_test.o : sht31_test.cpp sht31.h ../i2cdevice.h
	$(CPP) $(CFLAGS) sht31_test.cpp
	
sht31.o  : sht31.cpp sht31.h ../i2cdevice.h
	$(CPP) $(CFLAGS) sht31.cpp
	
i2cdevice.o : ../i2cdevice.cpp ../i2cdevice.h
	$(CPP) $(CFLAGS) ../i2cdevice.cpp
	
clean :	
	\rm -f *.o *~ $(OBJS)