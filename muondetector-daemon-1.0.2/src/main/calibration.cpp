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
	// before we write to eeprom, increase the write cycle counter
	CalibStruct item = getCalibItem("WRITE_CYCLES");
	uint32_t cycleCounter;
	getValueFromString(item.value, cycleCounter);
	cycleCounter++;
	setCalibItem("WRITE_CYCLES", cycleCounter);
	// write content of all calib parameters to buffer before actually writing to the eep		
	updateBuffer();
//	if (!isUpdated()) return true;
	bool success = fEeprom->writeBytes(0, 256, fEepBuffer);
	if (!success) { cerr<<"error: write to eeprom failed!"<<endl; return false; }
	if (fVerbose>1) cout<<"eep write took "<<fEeprom->getLastTimeInterval()<<" ms"<<endl;
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

