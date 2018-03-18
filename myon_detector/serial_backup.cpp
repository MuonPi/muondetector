#include "serial.h"
#include <string>
#include <iostream>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <iomanip>

#define VERBOSITY 0

using namespace std;


/* serial::serial(const std::string& portname)
{
  fd = open(portname, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1)
  {
		std::cerr<<"unable to open port "<<portname<<std::endl;
  }
  else
	fcntl(fd, F_SETFL, 0);
  //return (fd);
}
 */


Serial::Serial() {

	// Set up initial values
	fPortName = "/dev/ttyS0";
	fBaud = 9600;
	fDataBits = 8;
	fStopBits = 1;
	fParity = NONE;
	fBufSize = 1000;
	fBufIndex = 0;
	fBlocking = false;
	fVerbose = VERBOSITY;
	fd = -1;
}

Serial::Serial(const std::string& portname,
	int baud,
	unsigned char nrDataBits,
	unsigned char nrStopBits,
	int bufferSize,
	PARITY parity,
	bool blocking)
	: fPortName(portname), fBaud(baud), fDataBits(nrDataBits), fStopBits(nrStopBits), fBufSize(bufferSize), fParity(parity), fBlocking(blocking)
{
	fVerbose = VERBOSITY;
	fd = -1;
	fTimeout = 1; // timeout in tenths of seconds
	//fBufSize = 1000;
}


Serial::~Serial() {

	// Kills the port
	closePort();
}

int Serial::initPort(int baud,
	unsigned char nrDataBits,
	unsigned char nrStopBits,
	int bufferSize,
	PARITY parity,
	bool blocking)
{
	fBaud = baud;
	fDataBits = nrDataBits;
	fStopBits = nrStopBits;
	fParity = parity;
	fBufSize = bufferSize;
	fBufIndex = 0;
	fBlocking = blocking;

	if (fd != -1) closePort();
    return initPort();
}

int Serial::initPort() {

	// Try to open serial port with r/w access
	if (fVerbose > 0)
		std::cout << "SERIAL: Opening " << fPortName << " at " << fBaud << " baud...";

    if (fBlocking){
        //	  fd = ::open(fPortName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
		fd = ::open(fPortName.c_str(), O_RDWR);
    }else{
		fd = ::open(fPortName.c_str(), O_RDWR | O_NONBLOCK);
    }
	// As long as port is actually open...
	if (fd != -1) {

		// Share the good news
		if (fVerbose > 0)
		{
			std::cout << "OK" << std::endl;
			// Display blocking info
            if (fBlocking){
                if (fVerbose > 0){
					std::cout << "SERIAL: Blocking enabled" << std::endl;
                }else if (fVerbose > 0){
					std::cout << "SERIAL: Blocking disabled" << std::endl;
                }
            }
		}

		// Configure port settings
		//fcntl(fd, F_SETFL, FNDELAY);

		// Save current port settings so we don't corrupt anything on exit
		struct termios options;
		tcgetattr(fd, &options);

		// Convert integer baud to Baud type
		// Default to 9600 baud if none specified
		switch (fBaud) {
		case 50:
			cfsetispeed(&options, B50);
			cfsetospeed(&options, B50);
			break;
		case 75:
			cfsetispeed(&options, B75);
			cfsetospeed(&options, B75);
			break;
		case 110:
			cfsetispeed(&options, B110);
			cfsetospeed(&options, B110);
			break;
		case 134:
			cfsetispeed(&options, B134);
			cfsetospeed(&options, B134);
			break;
		case 150:
			cfsetispeed(&options, B150);
			cfsetospeed(&options, B150);
			break;
		case 200:
			cfsetispeed(&options, B200);
			cfsetospeed(&options, B200);
			break;
		case 300:
			cfsetispeed(&options, B300);
			cfsetospeed(&options, B300);
			break;
		case 600:
			cfsetispeed(&options, B600);
			cfsetospeed(&options, B600);
			break;
		case 1200:
			cfsetispeed(&options, B1200);
			cfsetospeed(&options, B1200);
			break;
		case 1800:
			cfsetispeed(&options, B1800);
			cfsetospeed(&options, B1800);
			break;
		case 2400:
			cfsetispeed(&options, B2400);
			cfsetospeed(&options, B2400);
			break;
		case 4800:
			cfsetispeed(&options, B4800);
			cfsetospeed(&options, B4800);
			break;
		case 9600:
		default:
			cfsetispeed(&options, B9600);
			cfsetospeed(&options, B9600);
			break;
		case 38400:
			cfsetispeed(&options, B38400);
			cfsetospeed(&options, B38400);
			break;
		case 57600:
			cfsetispeed(&options, B57600);
			cfsetospeed(&options, B57600);
			break;
		case 115200:
			cfsetispeed(&options, B115200);
			cfsetospeed(&options, B115200);
			break;
		case 230400:
			cfsetispeed(&options, B230400);
			cfsetospeed(&options, B230400);
			break;
		case 460800:
			cfsetispeed(&options, B460800);
			cfsetospeed(&options, B460800);
			break;
		case 500000:
			cfsetispeed(&options, B500000);
			cfsetospeed(&options, B500000);
			break;
		case 576000:
			cfsetispeed(&options, B576000);
			cfsetospeed(&options, B576000);
			break;
		case 921600:
			cfsetispeed(&options, B921600);
			cfsetospeed(&options, B921600);
			break;
		case 1000000:
			cfsetispeed(&options, B1000000);
			cfsetospeed(&options, B1000000);
			break;
		case 1152000:
			cfsetispeed(&options, B1152000);
			cfsetospeed(&options, B1152000);
			break;
		case 1500000:
			cfsetispeed(&options, B1500000);
			cfsetospeed(&options, B1500000);
			break;
		case 2000000:
			cfsetispeed(&options, B2000000);
			cfsetospeed(&options, B2000000);
			break;
		case 2500000:
			cfsetispeed(&options, B2500000);
			cfsetospeed(&options, B2500000);
			break;
		case 3000000:
			cfsetispeed(&options, B3000000);
			cfsetospeed(&options, B3000000);
			break;
		case 3500000:
			cfsetispeed(&options, B3500000);
			cfsetospeed(&options, B3500000);
			break;
		case 4000000:
			cfsetispeed(&options, B4000000);
			cfsetospeed(&options, B4000000);
			break;
		}

		// Set options for proper port settings
		//	ie. 8 Data bits, No parity, 1 stop bit
		options.c_cflag |= (CLOCAL | CREAD);

		switch (fParity) {
		case ODD:
			options.c_cflag |= PARENB;
			options.c_cflag |= PARODD;
			//				printf("SERIAL: parity set to ODD\n");
			break;
		case EVEN:
			options.c_cflag |= PARENB;
			options.c_cflag &= ~PARODD;
			//				printf("SERIAL: parity set to EVEN\n");
			break;
		default:
			// No parity
			options.c_cflag &= ~PARENB;
			//				options.c_cflag |= CS8;
			//				printf("SERIAL: No parity set\n");
			break;
		}

		switch (fDataBits)
		{
		case 8:
		default:
			options.c_cflag |= CS8;
			break;
		case 7:
			options.c_cflag |= CS7;
			break;
		case 6:
			options.c_cflag |= CS6;
			break;
		case 5:
			options.c_cflag |= CS5;
			break;
		}  //end of switch data_bits

		switch (fStopBits)
		{
		case 1:
		default:
			break;
		case 2:
			options.c_cflag |= CSTOPB;
			break;
		}


		// Turn off hardware flow control
		options.c_cflag &= ~CRTSCTS;

		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		options.c_lflag |= ICANON;
		// Write our changes to the port configuration
		options.c_cc[VTIME] = fTimeout;   /* inter-character timer */
		options.c_cc[VMIN] = 1;   /* blocking read until n chars received */
		tcsetattr(fd, TCSANOW, &options);
        return 0;
	}

	// There was a problem, let's tell the user what it was
	else {
		//cerr<<"FAIL"<<endl;
		perror("Error opening port:");
		closePort();
		//exit(1);
	}

	// Send back the public port file descriptor
	return fd;
}

bool Serial::open() {
	return (initPort() != -1);
}

void Serial::flushPort() {

	// If the port is actually open, flush it
	if (fd != -1) ioctl(fd, TCFLSH, 2);
}

void Serial::setTimeout(int timeout)
{
	fTimeout = timeout;
	return;
	struct termios options;
	tcgetattr(fd, &options);
	options.c_cc[VTIME] = timeout;   /* inter-character timer */
	options.c_cc[VMIN] = 1;   /* blocking read until n chars received */
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);
}


int Serial::getData(std::string& data, int nrBytes) {

	struct termios options;
	tcgetattr(fd, &options);
	options.c_cc[VTIME] = fTimeout;   /* inter-character timer */
 //    options.c_cc[VTIME]    = 1;   /* inter-character timer */
 //    options.c_cc[VMIN]     = 200;   /* blocking read until n chars received */
	options.c_cc[VMIN] = nrBytes;   /* blocking read until n chars received */
 //     tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);

	// If the port is actually open, read the data
	data.clear();
	int result = -1;
	if (fd != -1) {
		// Grab the data and return the number of bytes actually read
//		return read(fd, data, sizeof(data));
		char buf[nrBytes];
		result = ::read(fd, buf, nrBytes);
		if (result < 0) {
			// 		  cerr<<"error reading from serial port"<<endl;
			return 0;
		}
		if (result > 0) {
			data.resize(result);
			for (int i = 0; i < result; i++) data[i] = buf[i];
		}
	}
	// Port is closed!
	return result;
}

int Serial::read(std::string& data, int nrBytes)
{
	return getData(data, nrBytes);
}

int Serial::read(std::string& data, int nrBytes, int timeout)
{
	fTimeout = timeout;
	return getData(data, nrBytes);
}


int Serial::sendData(const std::string& data) {

	// If the port is actually open, send the data
	if (fd != -1) {
		// Send the data and return the number of bytes actually written
		//printf("Writing %s...\n", data);
		return ::write(fd, data.c_str(), data.size());
	}
	// Port is closed!
	else return -1;
}

int Serial::write(const std::string& data) {
	return sendData(data);
}

int Serial::readLine(std::string& data, char delim)
{
	/*
	   struct termios options;
	   tcgetattr(fd, &options);
	   options.c_cc[VTIME]    = fTimeout;   // inter-character timer
	   options.c_cc[VMIN]     = 0;   // blocking read until n chars received
	   tcflush(fd, TCIFLUSH);
	   tcsetattr(fd,TCSANOW,&options);
	*/
	data.clear();
	// If the port is actually open, grab a byte
	if (fd != -1) {
		while (true) {
			// Return the byte received if it's there
			int n = -1;
			int delay = 0;
			while (n < 1) {
				n = ::read(fd, &temp[0], 1);
				if (n == -1) usleep(100);   // wait 100us
				delay++;
				if (delay > fTimeout * 1000) {
					cerr << "Timeout" << endl;
					return data.size();
				}
			}
			//cout<<"delay="<<delay<<endl;
			if (n > 0) {
				//cout<<"n="<<n<<endl;
				for (int i = 0; i < n; i++) {
					//               data.push_back(temp[i]);
							   //cout<<" "<<temp[i];
					if (temp[i] == delim) {
						//cout<<"delim found"<<endl;
						return data.size();
					}
					data.push_back(temp[i]);
				}
			}
		}
	}
    return -1;
}

bool Serial::isDataAvailable(char& data)
{

	struct termios options;
	tcgetattr(fd, &options);
	options.c_cc[VTIME] = 0;   // inter-character timer
	options.c_cc[VMIN] = 1;   // blocking read until n chars received
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options);

	int result = -1;
	if (fd != -1) {
		// Grab the data and return the number of bytes actually read
//		return read(fd, data, sizeof(data));
		char buf[100];
		result = ::read(fd, buf, 1);
		if (result > 0) {
			data = buf[0];
			return true;
		}

	}
	// Port is closed!
	return false;

}

char Serial::getChar(int timeout_ms) {
	/*
	   struct termios options;
	   tcgetattr(fd, &options);
	   options.c_cc[VTIME]    = 0;   // inter-character timer
	   options.c_cc[VMIN]     = 1;   // blocking read until n chars received
	   tcflush(fd, TCIFLUSH);
	   tcsetattr(fd,TCSANOW,&options);
	*/
	int delay = 0;
	switch (fBlocking) {
		// Non-blocking mode enable, so use the buffer
	case false:
		if (fBufIndex > 0) {
			// Yes, so grab the byte we need and shift the buffer left
			char c = fBuf[1];
			for (int j = 1; j < (fBufIndex + 1); j++) {
				fBuf[j] = fBuf[j + 1];
			}
			fBufIndex--;
			// Send back the first byte in the buffer
			return c;
		}
		// If the port is actually open, grab a byte
		if (fd != -1) {
			// Return the byte received if it's there
			int n = ::read(fd, &temp[0], 1);
			if (n > -1) {
				// Just 1 byte, so return it right away
				if (n == 1) return temp[0];
				// More than 1 byte there, so buffer them and return the first
				else {
					for (int i = 0; i < (n - 1); i++) {
						fBufIndex++;
						fBuf[fBufIndex] = temp[i + 1];
					}
					return temp[0];
				}
			}
			else return -1;
		}
		break;
		// Blocking mode enabled, so wait until we get a byte
	case true:
		if (fBufIndex > 0) {
			// Yes, so grab the byte we need and shift the buffer left
			char c = fBuf[1];
			for (int j = 1; j < (fBufIndex + 1); j++) {
				fBuf[j] = fBuf[j + 1];
			}
			fBufIndex--;
			// Send back the first byte in the buffer
			return c;
		}
		// If the port is actually open, grab a byte
		if (fd != -1) {
			// Return the byte received if it's there
			int n = -1;
			delay = 0;
			while (n < 1) {
				n = ::read(fd, &temp[0], 1);
				if (n == -1) usleep(100);	// wait 1ms
				delay++;
				if (delay > timeout_ms * 10) return -1;
			}
			if (n > -1) {
				// Just 1 byte, so return it right away
				if (n == 1) return temp[0];
				// More than 1 byte there, so buffer them and return the first
				else {
					for (int i = 0; i < (n - 1); i++) {
						fBufIndex++;
						fBuf[fBufIndex] = temp[i + 1];
					}
					return temp[0];
				}
			}
			else return -1;
		}
		// Port is closed!
		else return -1;
		break;
		// Blocking variable is messed up
	default:
		cerr << "SERIAL: Error with blocking setting!" << endl;
		return -1;
		break;
	}
	// Should never get here
	return -1;
}

int Serial::sendChar(char data) {
	switch (fBlocking) {
		// Non-blocking mode
	case false:
		// If the port is actually open, send a byte
		if (fd != -1) {
			// Send the data and return number of bytes actually written
			::write(fd, &data, 1);
			return 1;
		}
		else return -1;
		break;
		// Blocking mode
	case true:
		// If the port is actually open, send a byte
		if (fd != -1) {
			// Send the data and return number of bytes actually written
			::write(fd, &data, 1);
			return 1;
		}
		else return -1;
		break;
		// Blocking variable is messed up
	default:
		cerr << "SERIAL: Error with blocking setting!" << endl;
		return -1;
		break;
	}
}

void Serial::closePort() {

	// If the port is actually open, close it
	if (fd != -1) {
		close(fd);
		fd = -1;
		if (fVerbose > 0)
			std::cout << "SERIAL: Device " << fPortName << " is now closed." << std::endl;
	}
}

