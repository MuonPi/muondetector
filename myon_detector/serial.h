#ifndef _SERIAL_H_
#define _SERIAL_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// STL includes
#include <string>

#define BUFFERSIZE	255

/**
 * This class handles low level serial port I/O
 */

class Serial {

public:

	enum PARITY { NONE, ODD, EVEN };

	/**
	 * Constructor.
	 * Initializes class variables.
	 */
	Serial();

	/**
	 * Constructor.
	 * Initializes port with given attributes
	 */
	Serial(const std::string&,
		int baud = 9600,
		unsigned char nrDataBits = 8,
		unsigned char nrStopBits = 1,
		int bufferSize = 1000,
		PARITY parity = NONE,
		bool blocking = true);

	/**
	 * Deconstructor.
	 * Called when class instance is destroyed.
	 */
	~Serial();

	/**
	 * Serial port initialization.
	 * Opens a port and returns the file descriptor
	 * @return int - file descriptor (value of -1 means init failed)
	 */
	int initPort();

	int initPort(
		int baud,
		unsigned char nrDataBits = 8,
		unsigned char nrStopBits = 1,
		int bufferSize = 1000,
		PARITY parity = NONE,
		bool blocking = true);

	/**
	 * opens a port
	 * @return bool - status, true if operation succeeded
	 */
	bool open();

	/**
	* returns the state of the port
	* @return bool - true if port is open
	*/
	bool isOpen() const { return (fd != -1); }

	/**
	 * Read the buffer.
	 * Polls the serial port for data and returns it to the host program
	 * @param data - string to hold the new data
   * @param nrBytes - number of bytes to read at most
   * @return number of bytes collected
	 */
	int getData(std::string& data, int nrBytes);

	/**
	 * Read the buffer.
	 * Polls the serial port for data and returns it to the host program
	 * @param data - string to hold the new data
	 * @param nrBytes - number of bytes to read at most
	 * @return number of bytes collected
	 */
	int read(std::string& data, int nrBytes);

	/**
	 * Read the buffer.
	 * Polls the serial port for data and returns it to the host program
	 * @param data - string to hold the new data
	 * @param nrBytes - number of bytes to read at most
	 * @return number of bytes collected
	 */
	int read(std::string& data, int nrBytes, int timeout);

	/**
	 * Read data from port until delimiter is received.
	 * Polls the serial port for data and returns it to the host program
	 * @param data - string to hold the new data
	 * @param delim - delimiter character
	 * @return
	 */
	int readLine(std::string& data, char delim = '\n');


	/**
	 * Writes to the buffer.
	 * @param data - string containing data to send
	 * @return int - number of bytes sent
	 */
	int sendData(const std::string& data);

	/**
	 * Writes to the buffer.
	 * @param data - string containing data to send
	 * @return int - number of bytes sent
	 */
	int write(const std::string& data);

	/**
	 * Reads a byte from the buffer.
   * @param timeout_ms - timeout for the read operation in ms
	 * @return char - ASCII value of byte read, -1 if no byte read.
	 */
	char getChar(int timeout_ms = 100);

	/**
	 * Sends a byte to the buffer.
	 * @param data - The char to send to serial port.
	 * @param return - Number of bytes sent.
	 */
	int sendChar(char data);

	/**
	 * Flush port.
	 * Clears all data that is currently pending in either the transmit or receive buffers.
	 */
	void flushPort();

	/**
	 * Close port.
	 * Serial port communication closed and port freed.
	 */
	void closePort();

	void setTimeout(int timeout);
	bool isDataAvailable(char& data);



private:
	int fd; /**< File descriptor for open port */
	std::string fPortName; /**< Device string for serial port */
	int fBaud; /**< Baud rate for serial port */
	unsigned char fDataBits; /**< Number of data bits for serial port */
	unsigned char fStopBits; /**< Number of stop bits for serial port */
	int fBufSize; /**< Sets the size of the input buffer */
	PARITY fParity; /**< parity setting of serial port */
	bool fBlocking; /**< Flag for opening port in blocking mode */
	char fBuf[256]; /**< Buffer to hold extra bytes received */
	char temp[256]; /**< Temporary storage for getChar() function */
	int fBufIndex; /**< Location for next byte in buffer */
	int fVerbose;
	int fTimeout;
};

#endif //_SERIAL_H
