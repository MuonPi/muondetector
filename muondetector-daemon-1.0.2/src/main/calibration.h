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
#include <calib_struct.h>
#include "i2c/i2cdevices.h"


const uint16_t CALIB_HEADER = 0x2019;

static const CalibStruct InvalidCalibStruct = CalibStruct( "", "", 0, "" );

// initialization of calib items
// meaning of entries (columns is:
// <item name> <item type> <default value>
static const std::vector<std::tuple<std::string, std::string, std::string>> CALIBITEMS = { 
													{"VERSION", "UINT8",  "1"} ,
													{"FEATURE_FLAGS", "UINT8",  "0"} ,
													{"CALIB_FLAGS", "UINT8", "0"} ,
													{"DATE", "UINT32", "0"} ,
													{"RSENSE", "UINT16", "100"} ,
													{"VDIV", "UINT16", "1100"} ,
													{"COEFF0", "FLOAT", "0.0"} ,
													{"COEFF1", "FLOAT", "1.0"} ,
													{"COEFF2", "FLOAT", "1.0"} ,
													{"COEFF3", "FLOAT", "1.0"} ,
													{"WRITE_CYCLES", "UINT32", "1"} ,
													/* std::make_tuple("CALIB_FLAGS", "UINT8", "1") 
													 * alternative, since list init formally introduced in C++17
													 * but it works with the current gnu c++ compiler with gnu++11 extensions.
													 * if your compiler is more strict, you should use init via make_tuple*/
													};
													

class ShowerDetectorCalib {

public:
	ShowerDetectorCalib() { init(); }
    ShowerDetectorCalib(EEPROM24AA02 *eep) : fEeprom(eep) { init(); }

	bool readFromEeprom();
	bool writeToEeprom();

	static uint8_t getTypeSize(const std::string& a_type);
	const std::vector<CalibStruct>& getCalibList() const { return fCalibList; }
	std::vector<CalibStruct>& calibList() { return fCalibList; }
	const CalibStruct& getCalibItem(unsigned int i) const { if (i<fCalibList.size()) return fCalibList[i]; else return InvalidCalibStruct; }
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

private:
	void init() {
		const uint16_t n=256;
		for (int i=0; i<n; i++) fEepBuffer[i]=0;
		buildCalibList();
		if (fEeprom != nullptr) {
			fEepromValid = fEeprom->devicePresent() && readFromEeprom();
		}
	}
	
	void buildCalibList() {
		fCalibList.clear();
		std::vector<std::tuple<std::string, std::string, std::string>>::const_iterator it;
		uint8_t addr=0x02;  // calib range starts at 0x02 behind header
		for (it=CALIBITEMS.begin(); it != CALIBITEMS.end(); it++) {
			CalibStruct calibItem;
			calibItem.name=std::get<0>(*it);
			calibItem.type=std::get<1>(*it);
			calibItem.address=addr;
			addr+=getTypeSize(std::get<1>(*it));
			calibItem.value=std::get<2>(*it);
			fCalibList.push_back(calibItem);
		}
	}
	
//	void updateBuffer();
	
	template <typename T>
	void getValueFromString(const std::string& valstr, T& value) { 
		//value = std::stoi(valstr, nullptr);
		std::istringstream istr(valstr);
		istr >> value;
	}

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
	CalibStruct item;
	item = getCalibItem(name);
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
