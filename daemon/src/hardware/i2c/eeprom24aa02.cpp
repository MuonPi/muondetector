#include "hardware/i2c/eeprom24aa02.h"
#include <stdint.h>
#include <unistd.h>
#include <thread>
#include <chrono>

/*
* 24AA02 EEPROM
*/

uint8_t EEPROM24AA02::readByte(uint8_t addr)
{
    uint8_t val = 0;
    startTimer();
    readReg(addr, &val, 1); // Read the data at address location
    stopTimer();
    return val;
}

bool EEPROM24AA02::readByte(uint8_t addr, uint8_t* value)
{
    startTimer();
    int n = readReg(addr, value, 1); // Read the data at address location
    stopTimer();
    return (n == 1);
}

void EEPROM24AA02::writeByte(uint8_t addr, uint8_t data)
{
    uint8_t writeBuf[2]; // Buffer to store the 2 bytes that we write to the I2C device

    writeBuf[0] = addr; // address of data byte
    writeBuf[1] = data; // data byte

    startTimer();

    // Write address first
    write(writeBuf, 2);

    usleep(5000);
    stopTimer();
}

bool EEPROM24AA02::writeBytes(uint8_t addr, uint16_t length, uint8_t* data)
{
    static const uint8_t PAGESIZE = 8;
    bool success = true;
    startTimer();
    for (uint16_t i = 0; i < length;) {
        uint8_t currAddr = addr + i;
        // determine, how many bytes left on current page
        uint8_t pageRemainder = PAGESIZE - currAddr % PAGESIZE;
        if (currAddr + pageRemainder >= length)
            pageRemainder = length - currAddr;
        int n = writeReg(currAddr, &data[i], pageRemainder);
        std::this_thread::sleep_for( std::chrono::microseconds( 5000 ) );
        i += pageRemainder;
        success = success && (n == pageRemainder);
    }
    stopTimer();
    return success;
}

int16_t EEPROM24AA02::readBytes(uint8_t regAddr, uint16_t length, uint8_t* data)
{
	return i2cDevice::readBytes( regAddr, length, data );
}

bool EEPROM24AA02::identify()
{
	if ( fMode == MODE_FAILED ) return false;
	if ( !devicePresent() ) return false;

	const unsigned int N { 256 };
	uint8_t buf[N+1];
//	std::cout << " attempt 1: offs=0, len="<<N<<std::endl;
	if ( readBytes( 0x00, N, buf ) != N) {
		// somehow did not read exact same amount of bytes as it should
		return false;
	}
//	std::cout << " attempt 2: offs=1, len="<<N<<std::endl;
	if ( readBytes( 0x01, N, buf ) != N) {
		// somehow did not read exact same amount of bytes as it should
		return false;
	}
//	std::cout << " attempt 3: offs=0, len="<<N+1<<std::endl;
	if ( readBytes( 0x00, N+1, buf ) != N+1) {
		// somehow did not read exact same amount of bytes as it should
		return false;
	}
//	std::cout << " attempt 4: offs=0xfa, len="<<int(6)<<std::endl;
	if ( readBytes( 0xfa, 6, buf ) != 6) {
		// somehow did not read exact same amount of bytes as it should
		return false;
	}

	// seems, we have a 24AA02 (or larger) at this point
	// additionaly check, whether it could be a 24AA02UID, 
	// i.e. if the last 6 bytes contain 2 bytes of vendor/device code and 4 bytes of unique id
	if ( buf[0] == 0x29 && buf[1] == 0x41 ) {
		fTitle = fName = "24AA02UID";
	}
	return true;
}
