#include "hardware/i2c/pca9536.h"
#include <iomanip>
#include <iostream>
#include <stdint.h>

/*
* PCA9536 4 pin I/O expander
*/

bool PCA9536::setOutputPorts(uint8_t portMask)
{
    unsigned char data = ~portMask;
    startTimer();
    if (1 != writeReg(REG::CONFIG, &data, 1)) {
        return false;
    }
    stopTimer();
    return true;
}

bool PCA9536::setOutputState(uint8_t portMask)
{
    startTimer();
    if (1 != writeReg(REG::OUTPUT, &portMask, 1)) {
        return false;
    }
    stopTimer();
    return true;
}

uint8_t PCA9536::getInputState()
{
    uint8_t inport = 0x00;
    startTimer();
    readReg(REG::INPUT, &inport, 1);
    stopTimer();
    return inport & 0x0f;
}

bool PCA9536::devicePresent()
{
    uint8_t inport = 0x00;
    // read input port
    return (1 == readReg(REG::INPUT, &inport, 1));
}

bool PCA9536::identify()
{
    if (fMode == MODE_FAILED)
        return false;
    if (!devicePresent())
        return false;
    uint8_t bytereg { 0 };
    /*	
	for (int i=0; i<256; i++) {
		if ( !readByte( static_cast<uint8_t>(i), &bytereg ) ) break;
		std::cout << "reg 0x"<<std::hex<<std::setfill('0')<<std::setw(2)<<i<<" : 0x"<<(int)bytereg<<std::endl;
	}
*/
    if (!readByte(static_cast<uint8_t>(REG::INPUT), &bytereg)) {
        // there was an error
        return false;
    }
    if ((bytereg & 0xf0) != 0xf0)
        return false;
    /*
	if ( !readByte( static_cast<uint8_t>(REG::OUTPUT), &bytereg ) ) {
		// there was an error
		return false;
	}
	if ( ( bytereg & 0xf0 ) != 0xf0 ) return false;
*/
    if (!readByte(static_cast<uint8_t>(REG::POLARITY), &bytereg)) {
        // there was an error
        return false;
    }
    if ((bytereg & 0xf0) != 0x00)
        return false;

    if (!readByte(static_cast<uint8_t>(REG::CONFIG), &bytereg)) {
        // there was an error
        return false;
    }
    if ((bytereg & 0xf0) != 0xf0)
        return false;
    if (readByte(0x04, &bytereg))
        return false;
    return true;
}
