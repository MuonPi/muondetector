#ifndef CALIBRATION_H
#define CALIBRATION_H

// for sig handling:
#include <sys/types.h>
#include <QObject>
#include <vector>
#include <tuple>
#include <sstream>
#include <string>
#include <iomanip>
#include <custom_io_operators.h>
#include <muondetector_structs.h>
#include "i2c/i2cdevices.h"

const uint16_t CALIB_HEADER = 0x2019;

struct CalibStruct;


//static const CalibStruct InvalidCalibStruct = CalibStruct( "", "", 0, "" );

// initialization of calib items
// meaning of entries (columns is:
// <item name> <item type> <default value>
static const std::vector<std::tuple<std::string, std::string, std::string>> CALIBITEMS = {
                                                    std::make_tuple("VERSION", "UINT8",  "3") ,
                                                    std::make_tuple("FEATURE_FLAGS", "UINT8",  "0") ,
                                                    std::make_tuple("CALIB_FLAGS", "UINT8", "0") ,
                                                    std::make_tuple("DATE", "UINT32", "0") ,
                                                    std::make_tuple("RSENSE", "UINT16", "100") ,
                                                    std::make_tuple("VDIV", "UINT16", "1100") ,
                                                    std::make_tuple("COEFF0", "FLOAT", "0.0") ,
                                                    std::make_tuple("COEFF1", "FLOAT", "1.0") ,
                                                    std::make_tuple("COEFF2", "FLOAT", "0.0") ,
                                                    std::make_tuple("COEFF3", "FLOAT", "1.0") ,
                                                    std::make_tuple("WRITE_CYCLES", "UINT32", "1")
                                                    };


class ShowerDetectorCalib {

public:
    static const CalibStruct InvalidCalibStruct;

    ShowerDetectorCalib() { init(); }
    ShowerDetectorCalib(EEPROM24AA02 *eep) : fEeprom(eep) { init(); }

    bool readFromEeprom();
    bool writeToEeprom();

    static uint8_t getTypeSize(const std::string& a_type);
    const std::vector<CalibStruct>& getCalibList() const { return fCalibList; }
    std::vector<CalibStruct>& calibList() { return fCalibList; }
    const CalibStruct& getCalibItem(unsigned int i) const;
    const CalibStruct& getCalibItem(const std::string& name);
    void setCalibItem(const std::string& name, const CalibStruct& item);
    template <typename T>
    void setCalibItem(const std::string& name, T value);
    void updateBuffer();
    void printBuffer();
    void printCalibList();
    bool isValid() const { return fValid; }
    bool isEepromValid() const { return fEepromValid; }
    uint64_t getSerialID();

    template <typename T>
    static void getValueFromString(const std::string& valstr, T& value) {
        //value = std::stoi(valstr, nullptr);
        std::istringstream istr(valstr);
        istr >> value;
    }

private:
    void init();
    void buildCalibList();

//	void updateBuffer();

    std::vector<CalibStruct> fCalibList;
    EEPROM24AA02 *fEeprom = nullptr;
    uint8_t fEepBuffer[256];
    bool fEepromValid = false;
    bool fValid = false;
    int fVerbose = 0;
};

// definition of member function template. Must reside in header
template <typename T>
void ShowerDetectorCalib::setCalibItem(const std::string& name, T value)
{
    CalibStruct item { getCalibItem(name) };
    //std::cout<<"*** ShowerDetectorCalib::setCalibItem(const std::string&, T) ***"<<std::endl;
    //std::cout<<"("<<item.name<<", "<<item.type<<", "<<(int)item.address<<", "<<item.value<<")"<<std::endl;

    if (item.name == name) {
        item.value = std::to_string(value);
        setCalibItem(name, item);
    }
}


template void ShowerDetectorCalib::setCalibItem<unsigned int>(const std::string&, unsigned int);
template void ShowerDetectorCalib::setCalibItem<int>(const std::string&, int);
//template void ShowerDetectorCalib::setCalibItem<float>(const std::string&, float);



#endif // CALIBRATION_H
