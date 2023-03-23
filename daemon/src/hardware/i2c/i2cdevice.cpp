#include "hardware/i2c/i2cdevice.h"
#include <algorithm>
#include <cstring>
#include <fcntl.h> // open
#include <iostream>
#include <linux/i2c-dev.h> // I2C bus definitions for linux like systems

using namespace std;

unsigned int i2cDevice::fNrDevices = 0;
unsigned long int i2cDevice::fGlobalNrBytesRead = 0;
unsigned long int i2cDevice::fGlobalNrBytesWritten = 0;
std::vector<i2cDevice*> i2cDevice::fGlobalDeviceList;

i2cDevice::i2cDevice()
{
    //opening the devicefile from the i2c driversection (open the device i2c)
    //"/dev/i2c-0" or "../i2c-1" for linux system. In our case
    fNrBytesRead = 0;
    fNrBytesWritten = 0;
    fAddress = 0;
    fDebugLevel = DEFAULT_DEBUG_LEVEL;
    fHandle = open("/dev/i2c-1", O_RDWR);
    if (fHandle > 0) {
        fNrDevices++;
        fGlobalDeviceList.push_back(this);
    } else
        fMode = MODE_FAILED;
}

i2cDevice::i2cDevice(const char* busAddress)
{
    fNrBytesRead = 0;
    fNrBytesWritten = 0;
    fDebugLevel = DEFAULT_DEBUG_LEVEL;
    //opening the devicefile from the i2c driversection (open the device i2c)
    //"/dev/i2c-0" or "../i2c-1" for linux system. In our case
    fHandle = open(busAddress, O_RDWR);
    if (fHandle > 0) {
        fNrDevices++;
        fGlobalDeviceList.push_back(this);
    } else
        fMode = MODE_FAILED;
}

i2cDevice::i2cDevice(uint8_t slaveAddress)
    : fAddress(slaveAddress)
{
    fNrBytesRead = 0;
    fNrBytesWritten = 0;
    fDebugLevel = DEFAULT_DEBUG_LEVEL;
    //opening the devicefile from the i2c driversection (open the device i2c)
    //"/dev/i2c-0" or "../i2c-1" for linux system. In our case
    fHandle = open("/dev/i2c-1", O_RDWR);
    if (fHandle > 0) {
        //ioctl(fHandle, I2C_SLAVE, fAddress);
        setAddress(slaveAddress);
        fNrDevices++;
        fGlobalDeviceList.push_back(this);
    } else
        fMode = MODE_FAILED;
}

i2cDevice::i2cDevice(const char* busAddress, uint8_t slaveAddress)
    : fAddress(slaveAddress)
{
    fNrBytesRead = 0;
    fNrBytesWritten = 0;
    fDebugLevel = DEFAULT_DEBUG_LEVEL;
    //opening the devicefile from the i2c driversection (open the device i2c)
    //"/dev/i2c-0" or "../i2c-1" for linux system. In our case
    fHandle = open(busAddress, O_RDWR);
    if (fHandle > 0) {
        setAddress(slaveAddress);
        fNrDevices++;
        fGlobalDeviceList.push_back(this);
    } else
        fMode = MODE_FAILED;
}

i2cDevice::~i2cDevice()
{
    //destructor of the opening part from above
    if (fHandle > 0) {
        fNrDevices--;
        close(fHandle);
    }
    std::vector<i2cDevice*>::iterator it;
    it = std::find(fGlobalDeviceList.begin(), fGlobalDeviceList.end(), this);
    if (it != fGlobalDeviceList.end())
        fGlobalDeviceList.erase(it);
}

void i2cDevice::getCapabilities()
{
    unsigned long funcs;
    int res = ioctl(fHandle, I2C_FUNCS, &funcs);
    if (res < 0)
        cerr << "error retrieving function capabilities from I2C interface." << endl;
    else {
        cout << "I2C adapter capabilities: 0x" << hex << funcs << dec << endl;
    }
}

bool i2cDevice::devicePresent()
{
    uint8_t dummy { 0 };
    return (read(&dummy, 1) == 1);
}

void i2cDevice::setAddress(uint8_t address)
{ //pointer to our device on the i2c-bus
    fAddress = address;
    int res = ioctl(fHandle, I2C_SLAVE, fAddress); //i.g. Specify the address of the I2C Slave to communicate with
    if (res < 0) {
        res = ioctl(fHandle, I2C_SLAVE_FORCE, fAddress);
        if (res < 0) {
            fMode = MODE_FAILED;
            fIOErrors++;
        } else
            fMode = MODE_FORCE;
    } else {
        fMode = MODE_NORMAL;
    }
}

void i2cDevice::lock(bool locked)
{
    if (locked)
        fMode |= MODE_LOCKED;
    else
        fMode &= ~MODE_LOCKED;
}

bool i2cDevice::identify()
{
    // this function should be reimplemented in derived classes
    // if it is not reimplemented it returns false, i.e. the abstract i2cDevice can never be identified positively
    // reimplement this method for specific devices and use whatever is needed to get an unanimous identification
    // return true if device is identified correctly
    return false;
}

int i2cDevice::read(uint8_t* buf, int nBytes)
{ //defines a function with a pointer buf as buffer and the number of bytes which
    //we want to read.
    if (fHandle <= 0 || (fMode & MODE_LOCKED))
        return 0;
    //std::lock_guard<std::mutex> lock(fMutex);
    int nread = ::read(fHandle, buf, nBytes); //"::" declares that the functions does not call itself again, but instead uses the system call
    if (nread > 0) {
        fNrBytesRead += nread;
        fGlobalNrBytesRead += nread;
        fMode &= ~((uint8_t)MODE_UNREACHABLE);
    } else if (nread <= 0) {
        fIOErrors++;
        fMode |= MODE_UNREACHABLE;
    }
    return nread;
}

int i2cDevice::write(uint8_t* buf, int nBytes)
{
    if (fHandle <= 0 || (fMode & MODE_LOCKED))
        return 0;
    //std::lock_guard<std::mutex> lock(fMutex);
    int nwritten = ::write(fHandle, buf, nBytes);
    if (nwritten > 0) {
        fNrBytesWritten += nwritten;
        fGlobalNrBytesWritten += nwritten;
        fMode &= ~((uint8_t)MODE_UNREACHABLE);
    } else if (nwritten <= 0) {
        fIOErrors++;
        fMode |= MODE_UNREACHABLE;
    }
    return nwritten;
}

int i2cDevice::writeReg(uint8_t reg, uint8_t* buf, int nBytes)
{
    // the i2c_smbus_*_i2c_block_data functions are better but allow
    // block sizes of up to 32 bytes only
    //i2c_smbus_write_i2c_block_data(int file, reg, nBytes, buf);

    uint8_t writeBuf[nBytes + 1];

    writeBuf[0] = reg; // first byte is register address
    for (int i = 0; i < nBytes; i++)
        writeBuf[i + 1] = buf[i];
    int n = write(writeBuf, nBytes + 1);
    return n - 1;
}

int i2cDevice::readReg(uint8_t reg, uint8_t* buf, int nBytes)
{
    std::lock_guard<std::mutex> lock(fMutex);
    int n = write(&reg, 1);
    if (n != 1)
        return -1;
    n = read(buf, nBytes);
    return n;
}

/** Read a single bit from an 8-bit device register.
* @param regAddr Register regAddr to read from
* @param bitNum Bit position to read (0-7)
* @param data Container for single bit value
* @return Status of read operation (true = success)
*/
int8_t i2cDevice::readBit(uint8_t regAddr, uint8_t bitNum, uint8_t* data)
{
    uint8_t b;
    uint8_t count = readReg(regAddr, &b, 1);
    *data = b & (1 << bitNum);
    return count;
}

/** Read multiple bits from an 8-bit device register.
* @param regAddr Register regAddr to read from
* @param bitStart First bit position to read (0-7)
* @param length Number of bits to read (not more than 8)
* @param data Container for right-aligned value (i.e. '101' read from any bitStart position will equal 0x05)
* @return Status of read operation (true = success)
*/
int8_t i2cDevice::readBits(uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t* data)
{
    // 01101001 read byte
    // 76543210 bit numbers
    //    xxx   args: bitStart=4, length=3
    //    010   masked
    //   -> 010 shifted
    uint8_t count, b;
    if ((count = readReg(regAddr, &b, 1)) != 0) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        b &= mask;
        b >>= (bitStart - length + 1);
        *data = b;
    }
    return count;
}

/** Read single byte from an 8-bit device register.
* @param regAddr Register regAddr to read from
* @param data Container for byte value read from device
* @return Status of read operation (true = success)
*/
bool i2cDevice::readByte(uint8_t regAddr, uint8_t* data)
{
    return (readBytes(regAddr, 1, data) == 1);
}

/** Read multiple bytes from an 8-bit device register.
* @param regAddr First register regAddr to read from
* @param length Number of bytes to read
* @param data Buffer to store read data in
* @return Number of bytes read (-1 indicates failure)
*/
int16_t i2cDevice::readBytes(uint8_t regAddr, uint16_t length, uint8_t* data)
{
    int nread {};
    size_t retry_ctr { 0 };
    while ((nread = readReg(regAddr, data, length)) != length && retry_ctr++ < fNrRetries) { };
    return nread;
}

/** write a single bit in an 8-bit device register.
* @param regAddr Register regAddr to write to
* @param bitNum Bit position to write (0-7)
* @param data New bit value to write
* @return Status of operation (true = success)
*/
bool i2cDevice::writeBit(uint8_t regAddr, uint8_t bitNum, uint8_t data)
{
    uint8_t b;
    std::lock_guard<std::mutex> lock(fMutex);
    int n = readByte(regAddr, &b);
    if (n != 1)
        return false;
    b = (data != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
    return writeByte(regAddr, b);
}

/** Write multiple bits in an 8-bit device register.
* @param regAddr Register regAddr to write to
* @param bitStart First bit position to write (0-7)
* @param length Number of bits to write (not more than 8)
* @param data Right-aligned value to write
* @return Status of operation (true = success)
*/
bool i2cDevice::writeBits(uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data)
{
    //      010 value to write
    // 76543210 bit numbers
    //    xxx   args: bitStart=4, length=3
    // 00011100 mask byte
    // 10101111 original value (sample)
    // 10100011 original & ~mask
    // 10101011 masked | value
    uint8_t b;
    std::lock_guard<std::mutex> lock(fMutex);
    if (readByte(regAddr, &b) != 0) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        data <<= (bitStart - length + 1); // shift data into correct position
        data &= mask; // zero all non-important bits in data
        b &= ~(mask); // zero all important bits in existing byte
        b |= data; // combine data with existing byte
        return writeByte(regAddr, b);
    } else {
        return false;
    }
}

/** Write single byte to an 8-bit device register.
* @param regAddr Register address to write to
* @param data New byte value to write
* @return Status of operation (true = success)
*/
bool i2cDevice::writeByte(uint8_t regAddr, uint8_t data)
{
    return writeBytes(regAddr, 1, &data);
}

/** Write multiple bytes to an 8-bit device register.
* @param devAddr I2C slave device address
* @param regAddr First register address to write to
* @param length Number of bytes to write
* @param data Buffer to copy new data from
* @return Status of operation (true = success)
*/
bool i2cDevice::writeBytes(uint8_t regAddr, uint16_t length, uint8_t* data)
{
    int nwritten {};
    size_t retry_ctr { 0 };
    while ((nwritten = writeReg(regAddr, data, length)) != length && retry_ctr++ < fNrRetries) { };
    return nwritten;
}

/** Write multiple words to a 16-bit device register.
* @param regAddr First register address to write to
* @param length Number of words to write
* @param data Buffer to copy new data from
* @return Status of operation (true = success)
*/
bool i2cDevice::writeWords(uint8_t regAddr, uint16_t length, uint16_t* data)
{
    int nwritten {};
    size_t retry_ctr { 0 };
    uint8_t buf[512];

    // Should do potential byteswap and call writeBytes() really, but that
    // messes with the callers buffer

    for (int i = 0; i < length; i++) {
        buf[i * 2] = data[i] >> 8;
        buf[i * 2 + 1] = data[i];
    }

    while ((nwritten = writeReg(regAddr, buf, length * 2)) != length * 2 && retry_ctr++ < fNrRetries) { };

    if (nwritten < 0) {
        //fprintf(stderr, "Failed to write to device(%d): %s\n", nwritten, ::strerror(errno));
        return false;
    } else if (nwritten != length * 2) {
        //fprintf(stderr, "Short write to device: expected %d, got %d\n", length * 2, nwritten);
        return false;
    }
    return true;
}

/** Write single word to a 16-bit device register.
* @param regAddr Register address to write to
* @param data New word value to write
* @return Status of operation (true = success)
*/
bool i2cDevice::writeWord(uint8_t regAddr, uint16_t data)
{
    return writeWords(regAddr, 1, &data);
}

/** Read single word from a 16-bit device register.
* @param regAddr Register regAddr to read from
* @param data Container for word value read from device
* @return Status of read operation (true = success)
*/
bool i2cDevice::readWord(uint8_t regAddr, uint16_t* data)
{
    return (readWords(regAddr, 1, data) == 1);
}

/** Read single word.
* @param data Container for word value read from device
* @return Status of read operation (true = success)
*/
bool i2cDevice::readWord(uint16_t* data)
{
    return (readWords(1, data) == 1);
}

/** Read multiple words from a 16-bit device register.
* @param regAddr First register regAddr to read from
* @param length Number of words to read
* @param data Buffer to store read data in
* @return Number of words read (-1 indicates failure)
*/
int16_t i2cDevice::readWords(uint8_t regAddr, uint16_t length, uint16_t* data)
{
    int nread {};
    size_t retry_ctr { 0 };
    uint8_t buf[512];

    while ((nread = readReg(regAddr, buf, length * 2)) != length * 2 && retry_ctr++ < fNrRetries) { };

    if (nread < 0) {
        //fprintf(stderr, "Failed to read device(%d): %s\n", nread, ::strerror(errno));
        return -1;
    } else if (nread != length * 2) {
        //fprintf(stderr, "Short read from device, expected %d, got %d\n", length * 2, nread);
        return nread / 2;
    }

    for (int i = 0; i < length; i++) {
        data[i] = buf[i * 2] << 8;
        data[i] |= buf[i * 2 + 1];
    }

    return nread / 2;
}

/** Read multiple words.
* @param length Number of words to read
* @param data Buffer to store read data in
* @return Number of words read (-1 indicates failure)
*/
int16_t i2cDevice::readWords(uint16_t length, uint16_t* data)
{
    int nread {};
    size_t retry_ctr { 0 };
    uint8_t buf[512];

    while ((nread = read(buf, length * 2)) != length * 2 && retry_ctr++ < fNrRetries) { };

    if (nread < 0) {
        //fprintf(stderr, "Failed to read device(%d): %s\n", nread, ::strerror(errno));
        return -1;
    } else if (nread != length * 2) {
        //fprintf(stderr, "Short read from device, expected %d, got %d\n", length * 2, nread);
        return nread / 2;
    }

    for (int i = 0; i < length; i++) {
        data[i] = buf[i * 2] << 8;
        data[i] |= buf[i * 2 + 1];
    }
    return nread / 2;
}

void i2cDevice::startTimer()
{
    gettimeofday(&fT1, NULL);
}

void i2cDevice::stopTimer()
{
    gettimeofday(&fT2, NULL);
    // compute and print the elapsed time in millisec
    double elapsedTime = (fT2.tv_sec - fT1.tv_sec) * 1000.0; // sec to ms
    elapsedTime += (fT2.tv_usec - fT1.tv_usec) / 1000.0; // us to ms
    fLastTimeInterval = elapsedTime;
    if (fDebugLevel > 1)
        printf(" last transaction took: %6.2f ms\n", fLastTimeInterval);
}
