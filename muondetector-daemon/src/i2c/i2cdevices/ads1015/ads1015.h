#ifndef _ADS1015_H_
#define _ADS1015_H_
#include "../ads1115/ads1115.h"

/* ADS1015: 4(2) ch, 12 bit ADC  */

class ADS1015 : public ADS1115 {
public:
	using ADS1115::ADS1115;
	//	enum CFG_RATE { RATE128 = 0, RATE250, RATE490, RATE920, RATE1600, RATE2400, RATE3300 };

protected:
	inline void init() {
		ADS1115::init();
		fTitle = "ADS1015";
	}
};

#endif // !_ADS1015_H_
