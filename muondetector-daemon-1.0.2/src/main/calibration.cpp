#include <iostream>
#include <cmath>
//#include <stdint.h>
#include "calibration.h"

#define AS_U32(f) (*(uint32_t*)&(f))
#define AS_U16(f) (*(uint16_t*)&(f))
#define AS_TIME(u) (*(time_t*)&(u))
#define AS_FLOAT(u) (*(float*)&(u))

using namespace std;

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
