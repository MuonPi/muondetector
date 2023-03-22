#include <fcntl.h> // open
#include <inttypes.h> // uint8_t, etc
#include <iostream>
#include <mutex>
#include <stdio.h>
#include <string>
#include <sys/ioctl.h> // ioctl
#include <sys/time.h> // for gettimeofday()
#include <unistd.h>
#include <vector>

#ifndef _I2CDEVICE_H_
#define _I2CDEVICE_H_

#define DEFAULT_DEBUG_LEVEL 0

// base class fragment static_device_base which implemets static functions available to all derived classes
// by the Curiously Recursive Template Pattern (CRTP) mechanism
template <class T>
struct static_device_base {
    friend class i2cDevice;
    static bool identifyDevice(uint8_t addr)
    {
        auto it = T::getGlobalDeviceList().begin();
        bool found { false };
        while (!found && it != T::getGlobalDeviceList().end()) {
            if ((*it)->getAddress() == addr) {
                found = true;
                break;
            }
            it++;
        }
        if (found) {
            T dummyDevice(0x00);
            if ((*it)->getTitle() == dummyDevice.getTitle())
                return true;
            return false;
        }
        T device(addr);
        return device.identify();
    }
};

//We define a class named i2cDevices to outsource the hardware dependent program parts. We want to
//access components of integrated curcuits, like the ads1115 or other subdevices via i2c-bus.
//The main aim here is, that the user does not have to be concerned about the c like low level operations
//of I2C access
// First, we define an abstract base class with all low-level i2c acess functions implemented.
// For device specific implementations, classes can inherit this base class
// virtual methods should be reimplemented in the child classes to make sense there, e.g. devicePresent()
class i2cDevice {
public:
    enum MODE { MODE_NONE = 0,
        MODE_NORMAL = 0x01,
        MODE_FORCE = 0x02,
        MODE_UNREACHABLE = 0x04,
        MODE_FAILED = 0x08,
        MODE_LOCKED = 0x10 };

    i2cDevice();
    i2cDevice(const char* busAddress = "/dev/i2c-1");
    i2cDevice(uint8_t slaveAddress);
    i2cDevice(const char* busAddress, uint8_t slaveAddress);
    virtual ~i2cDevice();

    void setAddress(uint8_t address);
    uint8_t getAddress() const { return fAddress; }
    static unsigned int getNrDevices() { return fNrDevices; }
    unsigned int getNrBytesRead() const { return fNrBytesRead; }
    unsigned int getNrBytesWritten() const { return fNrBytesWritten; }
    unsigned int getNrIOErrors() const { return fIOErrors; }
    static unsigned int getGlobalNrBytesRead() { return fGlobalNrBytesRead; }
    static unsigned int getGlobalNrBytesWritten() { return fGlobalNrBytesWritten; }
    static std::vector<i2cDevice*>& getGlobalDeviceList() { return fGlobalDeviceList; }
    virtual bool devicePresent();
    uint8_t getStatus() const { return fMode; }
    void lock(bool locked = true);

    double getLastTimeInterval() const { return fLastTimeInterval; }

    void setDebugLevel(int level) { fDebugLevel = level; }
    int getDebugLevel() const { return fDebugLevel; }

    void setTitle(const std::string& a_title) { fTitle = a_title; }
    const std::string& getTitle() const { return fTitle; }

    // read nBytes bytes into buffer buf
    // return value:
    // 	the number of bytes actually read if successful
    //	-1 on error
    int read(uint8_t* buf, int nBytes);

    // write nBytes bytes from buffer buf
    // return value:
    // 	the number of bytes actually written if successful
    //	-1 on error
    int write(uint8_t* buf, int nBytes);

    // write nBytes bytes from buffer buf in register reg
    // return value:
    // 	the number of bytes actually written if successful
    //	-1 on error
    // note: first byte of the write sequence is the register address,
    // the following bytes from buf are then written in a sequence
    int writeReg(uint8_t reg, uint8_t* buf, int nBytes);

    // read nBytes bytes into buffer buf from register reg
    // return value:
    // 	the number of bytes actually read if successful
    //	-1 on error
    // note: first writes reg address and after a repeated start
    // reads in a sequence of bytes
    // not all devices support this procedure
    // refer to the device's datasheet
    int readReg(uint8_t reg, uint8_t* buf, int nBytes);

    int8_t readBit(uint8_t regAddr, uint8_t bitNum, uint8_t* data);
    int8_t readBits(uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t* data);
    bool readByte(uint8_t regAddr, uint8_t* data);
    int16_t readBytes(uint8_t regAddr, uint16_t length, uint8_t* data);
    bool writeBit(uint8_t regAddr, uint8_t bitNum, uint8_t data);
    bool writeBits(uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data);
    bool writeByte(uint8_t regAddr, uint8_t data);
    bool writeBytes(uint8_t regAddr, uint16_t length, uint8_t* data);
    bool writeWords(uint8_t regAddr, uint16_t length, uint16_t* data);
    bool writeWord(uint8_t regAddr, uint16_t data);
    int16_t readWords(uint8_t regAddr, uint16_t length, uint16_t* data);
    int16_t readWords(uint16_t length, uint16_t* data);
    bool readWord(uint8_t regAddr, uint16_t* data);
    bool readWord(uint16_t* data);

    void getCapabilities();

    virtual bool identify();

protected:
    int fHandle { 0 };
    uint8_t fAddress { 0x00 };
    static unsigned int fNrDevices;
    unsigned long int fNrBytesWritten { 0 };
    unsigned long int fNrBytesRead { 0 };
    static unsigned long int fGlobalNrBytesRead;
    static unsigned long int fGlobalNrBytesWritten;
    double fLastTimeInterval { 0. }; // the last time measurement's result is stored here
    struct timeval fT1, fT2;
    int fDebugLevel { 0 };
    static std::vector<i2cDevice*> fGlobalDeviceList;
    std::string fTitle { "I2C device" };
    uint8_t fMode { MODE_NONE };
    unsigned int fIOErrors { 0 };
    std::mutex fMutex;
    static constexpr std::size_t fNrRetries { 3 };

    // fuctions for measuring time intervals
    void startTimer();
    void stopTimer();
};

#endif // _I2CDEVICE_H_
