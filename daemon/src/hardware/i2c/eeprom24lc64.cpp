#include "hardware/i2c/eeprom24lc64.h"
#include <chrono>
#include <stdint.h>
#include <thread>
#include <unistd.h>

/*
* 24LC64 EEPROM
*/

uint8_t EEPROM24LC64::readByte(uint16_t addr)
{
    std::lock_guard<std::mutex> lock(fMutex);
    uint8_t addr_array[2];
    addr_array[0] = static_cast<uint8_t>(addr >> 8);
    addr_array[1] = static_cast<uint8_t>(addr & 0xff);
    startTimer();
    int n = write(addr_array, 2);
    uint8_t buf[1];
    n = read(buf, 1); // Read the data at address location
    stopTimer();
    return buf[0];
}

void EEPROM24LC64::writeByte(uint16_t addr, uint8_t data)
{
    std::lock_guard<std::mutex> lock(fMutex);
    uint8_t writeBuf[3]; // Buffer to store the 3 bytes that we write to the I2C device

    writeBuf[0] = static_cast<uint8_t>(addr >> 8); // address of data byte
    writeBuf[1] = static_cast<uint8_t>(addr & 0xff);
    writeBuf[2] = data; // data byte

    startTimer();

    // Write address first
    write(writeBuf, 3);

    usleep(5000);
    stopTimer();
}

int EEPROM24LC64::writeReg(uint16_t reg, uint8_t* buf, int nBytes)
{
    // the i2c_smbus_*_i2c_block_data functions are better but allow
    // block sizes of up to 32 bytes only
    //i2c_smbus_write_i2c_block_data(int file, reg, nBytes, buf);

    uint8_t* wbuf = new uint8_t[nBytes + 2];
    if (!wbuf)
        return 0;
    int n { 0 };
    try {
        std::copy(buf, buf + nBytes, wbuf + 2);
        wbuf[0] = static_cast<uint8_t>(reg >> 8);
        wbuf[1] = static_cast<uint8_t>(reg & 0xff);
        n = write(wbuf, nBytes + 2);
    } catch (...) {
    }
    delete[] wbuf;
    return n - 2;
}
bool EEPROM24LC64::writeBytes(uint16_t addr, uint16_t length, uint8_t* data)
{
    std::lock_guard<std::mutex> lock(fMutex);
    static const uint8_t PAGESIZE = 32;
    bool success = true;
    startTimer();
    for (uint16_t i = 0; i < length;) {
        uint16_t currAddr = addr + i;
        // determine, how many bytes left on current page
        uint16_t pageRemainder = PAGESIZE - currAddr % PAGESIZE;
        if (currAddr + pageRemainder >= length)
            pageRemainder = length - currAddr;
        int n = writeReg(currAddr, &data[i], pageRemainder);
        std::this_thread::sleep_for(std::chrono::microseconds(5000));
        i += pageRemainder;
        success = success && (n == pageRemainder);
    }
    stopTimer();
    return success;
}

int16_t EEPROM24LC64::readBytes(uint16_t addr, uint16_t length, uint8_t* data)
{
    std::lock_guard<std::mutex> lock(fMutex);
    uint8_t addr_array[2];
    addr_array[0] = static_cast<uint8_t>(addr >> 8);
    addr_array[1] = static_cast<uint8_t>(addr & 0xff);
    startTimer();
    int n = write(addr_array, 2);
    if (n != 2)
        return -1;
    n = read(data, length);
    stopTimer();
    return n;
}

bool EEPROM24LC64::identify()
{
    if (fMode == MODE_FAILED)
        return false;
    if (!devicePresent())
        return false;

    const unsigned int N { 256 };
    uint8_t buf[N + 1];
    //	std::cout << " attempt 1: offs=0, len="<<N<<std::endl;
    if (readBytes(static_cast<uint16_t>(0x00), N, buf) != N) {
        // somehow did not read exact same amount of bytes as it should
        return false;
    }
    //	std::cout << " attempt 2: offs=1, len="<<N<<std::endl;
    if (readBytes(static_cast<uint16_t>(0x01), N, buf) != N) {
        // somehow did not read exact same amount of bytes as it should
        return false;
    }
    //	std::cout << " attempt 3: offs=0, len="<<N+1<<std::endl;
    if (readBytes(static_cast<uint16_t>(0x00), N + 1, buf) != N + 1) {
        // somehow did not read exact same amount of bytes as it should
        return false;
    }
    //	std::cout << " attempt 4: offs=0xfa, len="<<int(6)<<std::endl;
    if (readBytes(static_cast<uint16_t>(0xfa), 6, buf) != 6) {
        // somehow did not read exact same amount of bytes as it should
        return false;
    }
    return true;
}
