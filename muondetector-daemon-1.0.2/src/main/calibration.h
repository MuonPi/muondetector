#ifndef CALIBRATION_H
#define CALIBRATION_H

// for sig handling:
#include <sys/types.h>
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
													{"RSENSE", "UINT16", "200"} ,
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

/*
// make_string
class make_string {
public:
  template <typename T>
  make_string& operator<<( T const & val ) {
    buffer_ << val;
    return *this;
  }
  operator std::string() const {
    return buffer_.str();
  }
private:
  std::ostringstream buffer_;
};
*/









template <typename T, uint8_t ADDR> 
class CalibEntry
{
public:
	typedef T type;
	CalibEntry() : value() { }
	CalibEntry(const T& val) { value = val;	}
	const T& operator()() const { return value; }
	void setValue(const T& val) { value=val; updated=false; }
	void newValue(const T& val) { value=val; updated=true; }
	bool isUpdated() const { return updated; }
	void setUpdated(bool upd=true) { updated=upd; }
	uint8_t getAddress() const { return ADDR; }
	uint8_t getSize() const { return sizeof(T); }
private:
	T value = T();
	bool updated = false; // this flag keeps track if a value is updated wrt to the stored value
};


class Calibration
{

public:
	enum CALIB_ADDRESS_MAP {ADDR_HEADER=0x00, ADDR_VERSION=0x02,
							ADDR_FEATURE_FLAGS=0x03, ADDR_CALIB_FLAGS=0x04,
							ADDR_DATE=0x05,	ADDR_RSENSE=0x09, 
							ADDR_VDIV=0x0b, ADDR_COEFF0=0x0d,
							ADDR_COEFF1=ADDR_COEFF0+sizeof(float)};

	enum CALIB_FLAGS {NO_CALIB=0, COMPONENT_CALIB=0x01, BIAS_CALIB=0x02, ENERGY_CALIB=0x04};
	
	Calibration(EEPROM24AA02 *eep);
	~Calibration();
	bool readFromEeprom();
	bool writeToEeprom();
	const std::string& getID();
	bool isUpdated() const;
	bool isValid() const { return valid; }
	bool isEepromValid() const { return eepromValid; }
	uint8_t getVersion() const { return version(); }
	uint8_t getCalibFlags() const { return calib_flags(); }
	uint8_t getFeatureFlags() const { return feature_flags(); }
	time_t getDate() const { return (time_t)date(); }
	float getRsense() const { return (float)rsense()/10.; }
	float getVdiv() const { return (float)vdiv_ratio()/100.; }
	float getCoeff0() const;
	float getCoeff1() const;
	
	void setVersion(uint8_t a_version) { version.newValue(a_version); updateBuffer(); }
	void setCalibFlags(uint8_t flags) { calib_flags.newValue(flags); updateBuffer(); }
	void setFeatureFlags(uint8_t flags) { feature_flags.newValue(flags); updateBuffer(); }
	void setDate(time_t a_date) { date.newValue((uint32_t)a_date); updateBuffer(); }
	void setRsense(float a_rsense) { rsense.newValue((uint16_t)(a_rsense*10.)); updateBuffer(); }
	void setVdiv(float a_vdiv) { vdiv_ratio.newValue((uint16_t)(a_vdiv*100.)); updateBuffer(); }
	void setCoeff0(float a_coeff);
	void setCoeff1(float a_coeff);
	void printBuffer();
	
private:
	
	void updateBuffer();
	
	EEPROM24AA02 *eeprom = nullptr;
	bool valid = false;
	bool eepromValid = false;
	int verbose=1;
	uint8_t eep_buffer[256];
	const CalibEntry<uint16_t,ADDR_HEADER> header=0x2019;
	CalibEntry<uint8_t,ADDR_VERSION> version=0x01;
	CalibEntry<uint8_t,ADDR_CALIB_FLAGS> calib_flags;
	CalibEntry<uint8_t,ADDR_FEATURE_FLAGS> feature_flags;
	CalibEntry<uint32_t,ADDR_DATE> date=0; // calib date in unix seconds
	CalibEntry<uint16_t,ADDR_RSENSE> rsense=200; // Rsense series resistor value for current sense in 100 Ohm units (20kOhm => 200)
	CalibEntry<uint16_t,ADDR_VDIV> vdiv_ratio=1100; // voltage divider ratio of current sense leads in 1/100 units (10.4 => 1040)
	CalibEntry<float,ADDR_COEFF0> coeff0=0;
	CalibEntry<float,ADDR_COEFF1> coeff1=1.0;
	
	std::vector<CalibStruct> fCalibList;
};


#endif // CALIBRATION_H
