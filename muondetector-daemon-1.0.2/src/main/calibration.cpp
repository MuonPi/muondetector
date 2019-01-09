#include <iostream>
#include <cmath>
#include <algorithm>
#include <cctype>
//#include <stdint.h>
#include "calibration.h"

#define AS_U32(f) (*(uint32_t*)&(f))
#define AS_U16(f) (*(uint16_t*)&(f))
#define AS_TIME(u) (*(time_t*)&(u))
#define AS_I8(f) (*(int8_t*)&(f))
#define AS_I32(f) (*(int32_t*)&(f))
#define AS_I16(f) (*(int16_t*)&(f))
#define AS_FLOAT(u) (*(float*)&(u))
#define AS_FLOAT_C(u) (*(const float*)&(u))

using namespace std;

uint8_t ShowerDetectorCalib::getTypeSize(const std::string& a_type)
{
	string str=a_type;
	std::transform(a_type.begin(), a_type.end(), str.begin(), 
                   [](unsigned char c){ return std::toupper(c); }
                  );
	//cout<<"type str = "<<a_type<<" ; upper = "<<str<<endl;
	uint8_t size=0;
	if (str=="UINT8") size=1;
	else if (str=="UINT16") size=2;
	else if (str=="UINT32") size=4;
	else if (str=="FLOAT") size=4;
	else if (str=="INT8") size=1;
	else if (str=="INT16") size=2;
	else if (str=="INT32") size=4;
	
	return size;
}

const CalibStruct& ShowerDetectorCalib::getCalibItem(const std::string& name)
{
	string str=name;
	std::transform(name.begin(), name.end(), str.begin(), 
                   [](unsigned char c){ return std::toupper(c); }
                  );
	auto result = std::find_if(fCalibList.begin(), fCalibList.end(), [&str](const CalibStruct& item){ return item.name==str; } );
	//cout<<"*** ShowerDetectorCalib::getCalibItem(const std::string&) ***"<<endl;
	//cout<<"("<<result->name<<", "<<result->type<<", "<<(int)result->address<<", "<<result->value<<")"<<endl;
	if (result != std::end(fCalibList))	return *result; 
	else return InvalidCalibStruct;
}

void ShowerDetectorCalib::setCalibItem(const std::string& name, const CalibStruct& item)
{
	string str=name;
	std::transform(name.begin(), name.end(), str.begin(), 
                   [](unsigned char c){ return std::toupper(c); }
                  );
	auto result = std::find_if(fCalibList.begin(), fCalibList.end(), [&str](const CalibStruct& s){ return s.name==str; } );
	
	//std::cout<<"*** ShowerDetectorCalib::setCalibItem(const std::string&, const CalibStruct&) ***"<<std::endl;
	//cout<<"name="<<name<<" str="<<str<<endl;
	if (result != std::end(fCalibList)) {
		*result=item;
		//cout<<"("<<result->name<<", "<<result->type<<", "<<(int)result->address<<", "<<result->value<<")"<<endl;
	}
	//else cout<<"no match"<<endl;
	
}

bool ShowerDetectorCalib::readFromEeprom()
{
	if (fEeprom == nullptr) return false;
	//if (!fEeprom->devicePresent()) return false;
	const uint16_t n=256;
	for (int i=0; i<n; i++) fEepBuffer[i]=0;
	bool success=(fEeprom->readBytes(0,n,fEepBuffer)==n);
	if (!success) { fEepromValid = false; return false; }
	uint16_t eepheader = AS_U16(fEepBuffer[0]);
	if (eepheader==CALIB_HEADER) fValid=true;
	else { fValid=false; return true; }

	for (auto it=fCalibList.begin(); it!=fCalibList.end(); it++) {
		uint8_t addr = it->address;
		string str = it->type;
		std::ostringstream ostr;
		//cout<<"calibList entry: "<<it->name<<" = ";
		if (str=="UINT8") {
			uint8_t val = fEepBuffer[it->address];
			it->value = std::to_string(val);
		} else if (str=="UINT16") {
			uint16_t val = AS_U16(fEepBuffer[it->address]);
			it->value = std::to_string(val);
		} else if (str=="UINT32") {
			uint32_t val = AS_U32(fEepBuffer[it->address]);
			it->value = std::to_string(val);
		} else if (str=="FLOAT") {
			uint32_t _x = AS_U32(fEepBuffer[it->address]);
			if (fVerbose>5) cout<<"as U32="<<_x<<" ";
			float val = AS_FLOAT_C(_x);
			if (fVerbose>5) cout<<"as FLOAT="<<val<<" ";
			//setCalibItem(it->name, AS_FLOAT((fEepBuffer[addr])));
			//float* val = reinterpret_cast<float*>(&fEepBuffer[it->address]);
			//float val = 0.;
			// use of std::to_string() is not recommended, since it messes with the locale's setting, e.g. the decimal separator
			//it->value = std::to_string(val);
			ostr << std::setprecision(7) << std::scientific << val;
			it->value = ostr.str();
		} else if (str=="INT8") {
			int8_t val = AS_I8(fEepBuffer[it->address]);
			it->value = std::to_string(val);
		} else if (str=="INT16") {
			int16_t val = AS_I16(fEepBuffer[it->address]);
			it->value = std::to_string(val);
		} else if (str=="INT32") {
			int32_t val = AS_I32(fEepBuffer[it->address]);
			it->value = std::to_string(val);
		}
		else {
		}
		//cout<<it->value<<endl;
	}
	return true;
}

bool ShowerDetectorCalib::writeToEeprom()
{
	if (!fEepromValid) return false;
	updateBuffer();
//	if (!isUpdated()) return true;
	bool success = fEeprom->writeBytes(0, 256, fEepBuffer);
	if (!success) { cerr<<"error: write to eeprom failed!"<<endl; return false; }
	if (fVerbose>0) cout<<"eep write took "<<fEeprom->getLastTimeInterval()<<" ms"<<endl;
	// reset update flags of all properties since we just wrote them freshly into the EEPROM
	return true;
}

void ShowerDetectorCalib::updateBuffer()
{
	// write fixed header
	AS_U16(fEepBuffer[0])=CALIB_HEADER;
	
	for (auto it=fCalibList.begin(); it!=fCalibList.end(); it++) {
		uint8_t addr = it->address;
		string str = it->type;
		if (str=="UINT8") {
			unsigned int val;
			getValueFromString(it->value, val);
			fEepBuffer[it->address]=(uint8_t)val;
		} else if (str=="UINT16") {
			unsigned int val;
			getValueFromString(it->value, val);
			AS_U16(fEepBuffer[it->address])=(uint16_t)val;
		} else if (str=="UINT32") {
			unsigned int val;
			getValueFromString(it->value, val);
			AS_U32(fEepBuffer[it->address])=(uint32_t)val;
		} else if (str=="FLOAT") {
			float val;
			getValueFromString(it->value, val);
			AS_FLOAT(fEepBuffer[it->address])=val;
		} else if (str=="INT8") {
			int val;
			getValueFromString(it->value, val);
			AS_I8(fEepBuffer[it->address])=(int8_t)val;
		} else if (str=="INT16") {
			int val;
			getValueFromString(it->value, val);
			AS_I16(fEepBuffer[it->address])=(int16_t)val;
		} else if (str=="INT32") {
			int val;
			getValueFromString(it->value, val);
			AS_I32(fEepBuffer[it->address])=(int32_t)val;
		}
		else {
		}
	}

}

void ShowerDetectorCalib::printBuffer()
{
//	bool retval=(eep->readBytes(0,n,buf)==n);
	cout<<"*** Calibration buffer content ***"<<endl;
	for (int j=0; j<16; j++) {
		cout<<hex<<std::setfill ('0') << std::setw (2)<<j*16<<": ";
		for (int i=0; i<16; i++) {
			cout<<hex<<std::setfill ('0') << std::setw (2)<<(int)fEepBuffer[j*16+i]<<" ";
		}
		cout<<endl;
	}
	cout<<dec;
}

void ShowerDetectorCalib::printCalibList()
{
	cout<<"*** Calibration parameters ***"<<dec<<endl;
	int i=0;
	for (auto it=fCalibList.begin(); it!=fCalibList.end(); it++) {
		cout<<++i<<": "<<it->name;
		if (it->type=="FLOAT") {
			float val;
			getValueFromString(it->value,val);
			cout<<" \t= "<<std::setprecision(8)<<val<<" '"<<it->value<<"'"<<" ("<<it->type<<") @"<<(int)it->address<<endl; 
		} else cout<<" \t= "<<it->value<<" ("<<it->type<<") @"<<(int)it->address<<endl; 
	}
}


// partial specialization of member function template. Must reside OUTSIDE header file
template <>
void ShowerDetectorCalib::setCalibItem<float>(const std::string& name, float value)
{
	CalibStruct item;
	item = getCalibItem(name);
//	std::cout<<"*** ShowerDetectorCalib::setCalibItem(const std::string&, T) ***"<<std::endl;
//	std::cout<<"("<<item.name<<", "<<item.type<<", "<<(int)item.address<<", "<<item.value<<")"<<std::endl;

//	if (const_cast<const CalibStruct&>(item) != InvalidCalibStruct) { 
	if (item.name == name) { 
		std::ostringstream ostr;
		ostr<<std::setprecision(7);
		ostr << std::scientific << (double)value;
		item.value = ostr.str();
		setCalibItem(name, item);
	}
}

uint64_t ShowerDetectorCalib::getSerialID()
{
	// we assume that the unique ID is stored in the six last bytes of the EEPROM memory
	uint64_t id = 0x0000000000000000;
	id = fEepBuffer[255];
	id |= (uint64_t)fEepBuffer[254]<<8;
	id |= (uint64_t)fEepBuffer[253]<<16;
	id |= (uint64_t)fEepBuffer[252]<<24;
	id |= (uint64_t)fEepBuffer[251]<<32;
	id |= (uint64_t)fEepBuffer[250]<<40;
	return id;
}








Calibration::Calibration(EEPROM24AA02 *eep)
{
	eeprom = eep;
	if (eeprom != nullptr) {
		eepromValid = eeprom->devicePresent() && readFromEeprom();
	}
}

Calibration::~Calibration()
{
	
	
}

bool Calibration::readFromEeprom()
{
	uint16_t n=256;
	for (int i=0; i<n; i++) eep_buffer[i]=0;
	bool success=(eeprom->readBytes(0,n,eep_buffer)==n);
	if (!success) return false;
	uint16_t eepheader = AS_U16(eep_buffer[ADDR_HEADER]);
	if (eepheader==header()) valid=true;
	else { valid=false; return true; }
	
	version.setValue(eep_buffer[ADDR_VERSION]);
	calib_flags.setValue(eep_buffer[ADDR_CALIB_FLAGS]);
	feature_flags.setValue(eep_buffer[ADDR_FEATURE_FLAGS]);
//	date.setValue(*(eep_buffer+ADDR_DATE));
	date.setValue(AS_U32(eep_buffer[ADDR_DATE]));
	if (calib_flags() & COMPONENT_CALIB) {
		rsense.setValue(AS_U16(eep_buffer[ADDR_RSENSE]));
		vdiv_ratio.setValue(AS_U16(eep_buffer[ADDR_VDIV]));
	}
	if (calib_flags() & BIAS_CALIB) {
		coeff0.setValue(AS_FLOAT(eep_buffer[ADDR_COEFF0]));
		coeff1.setValue(AS_FLOAT(eep_buffer[ADDR_COEFF1]));
	}
	
	return true;
}

bool Calibration::writeToEeprom()
{
	if (!eepromValid) return false;
	if (!isUpdated()) return true;
	bool success = eeprom->writeBytes(0, 256, eep_buffer);
	if (!success) { cerr<<"error: write to eeprom failed!"<<endl; return false; }
	if (verbose>0) cout<<"eep write took "<<eeprom->getLastTimeInterval()<<" ms"<<endl;
	// reset update flags of all properties since we just wrote them freshly into the EEPROM
	version.setUpdated(false);
	calib_flags.setUpdated(false);
	feature_flags.setUpdated(false);
	date.setUpdated(false);
	rsense.setUpdated(false);
	vdiv_ratio.setUpdated(false);
	coeff0.setUpdated(false);
	coeff1.setUpdated(false);
	return true;
}

void Calibration::updateBuffer()
{
	AS_U16(eep_buffer[ADDR_HEADER])=header();
	eep_buffer[ADDR_VERSION]=version();
	eep_buffer[ADDR_CALIB_FLAGS]=calib_flags();
	eep_buffer[ADDR_FEATURE_FLAGS]=feature_flags();
	AS_U32(eep_buffer[ADDR_DATE])=date();
	AS_U16(eep_buffer[ADDR_RSENSE])=rsense();
	AS_U16(eep_buffer[ADDR_VDIV])=vdiv_ratio();
	AS_FLOAT(eep_buffer[ADDR_COEFF0]) = coeff0();
	AS_FLOAT(eep_buffer[ADDR_COEFF1]) = coeff1();
}

bool Calibration::isUpdated() const
{
	bool updated = false;
	updated = 	version.isUpdated() || calib_flags.isUpdated() || feature_flags.isUpdated() ||
				date.isUpdated() || rsense.isUpdated() || vdiv_ratio.isUpdated() ||
				coeff0.isUpdated() || coeff1.isUpdated();
	return updated;
}

float Calibration::getCoeff0() const
{

/*	int32_t rawValue = coeff0();
	double mant = (rawValue & 0xffffff00)/256.;
	int8_t expon = int8_t(rawValue & 0xff);
	float value = pow((float)mant, (float)expon);
*/
	float value=coeff0();
	return value;
}

float Calibration::getCoeff1() const
{
/* 	int32_t rawValue = coeff1();
	double mant = (rawValue & 0xffffff00)/256.;
	int8_t expon = int8_t(rawValue & 0xff);
	float value = pow((float)mant, (float)expon);
*/
	float value=coeff1();
	return value;
}

void Calibration::setCoeff0(float a_coeff)
{
	coeff0.newValue(a_coeff);
	updateBuffer();
}

void Calibration::setCoeff1(float a_coeff)
{
	coeff1.newValue(a_coeff);
	updateBuffer();
}

void Calibration::printBuffer()
{
//	bool retval=(eep->readBytes(0,n,buf)==n);
	cout<<"*** Calibration buffer content ***"<<endl;
	for (int j=0; j<16; j++) {
		cout<<hex<<std::setfill ('0') << std::setw (2)<<j*16<<": ";
		for (int i=0; i<16; i++) {
			cout<<hex<<std::setfill ('0') << std::setw (2)<<(int)eep_buffer[j*16+i]<<" ";
		}
		cout<<endl;
	}
}
