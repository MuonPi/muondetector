#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <custom_io_operators.h>
#include "i2c/i2cdevices.h"

// for sig handling:
#include <sys/types.h>


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
	
};

#endif // CALIBRATION_H
