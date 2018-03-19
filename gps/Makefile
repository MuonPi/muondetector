CC=g++
CFLAGS= -O -std=gnu++11
LDFLAGS= -std=gnu++11 -O -pthread
OBJS= ublox.o uconfig.o serial.o
uconfig: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o uconfig
	
uconfig.o: uconfig.cpp ublox.h
	$(CC) $(CFLAGS) -c uconfig.cpp
	
ublox.o: ublox.cpp ublox.h serial.h
	$(CC) $(CFLAGS) -c ublox.cpp
	
serial.o: serial.cpp serial.h
	$(CC) $(CFLAGS) -c serial.cpp
	
clean:
	rm -f *.o uconfig
