#ifndef _MIC184_H_
#define _MIC184_H_
#include "hardware/i2c/i2cdevice.h"
#include "hardware/i2c/lm75.h"

class MIC184 : public LM75, public static_device_base<MIC184> {
public:
    MIC184();
    MIC184(const char* busAddress, uint8_t slaveAddress);
    MIC184(uint8_t slaveAddress);
	virtual ~MIC184();
    bool identify() override;
	bool isExternal() const { return fExternal; }
	bool setExternal( bool enable_external = true );
private:
	bool fExternal { false };
};
#endif // !_MIC184_H_
