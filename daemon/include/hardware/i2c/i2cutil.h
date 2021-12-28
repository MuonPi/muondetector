#ifndef _I2CUTIL_H_
#define _I2CUTIL_H_

#include "hardware/i2c/i2cdevice.h"
#include <vector>
#include <iostream>
#include <iomanip>
#include <set>

class I2cGeneralCall : private i2cDevice {
public:
	static void resetDevices();
private:
	I2cGeneralCall();
	I2cGeneralCall(const char* busAddress);
};

template <class T>
std::vector <uint8_t> findI2cDeviceType( const std::set<uint8_t>& possible_addresses = std::set<uint8_t>() ) {
	std::vector<uint8_t> addr_list { };
	for ( uint16_t addr = 0x04; addr <= 0x78; addr++) {
		if ( !possible_addresses.empty() && possible_addresses.find( static_cast<uint8_t>( addr ) ) == possible_addresses.cend() ) continue;
		bool ident { false };
		ident = T::identifyDevice( static_cast<uint8_t>(addr) );
		if ( ident ) {
//			std::cout<<"device identified at 0x"<<std::setw(2) << std::setfill('0')<<std::hex<<(int)addr<<"\n";
			addr_list.push_back(static_cast<uint8_t>(addr));
		}
	}
	return addr_list;
}

#endif // !_I2CUTIL_H_
