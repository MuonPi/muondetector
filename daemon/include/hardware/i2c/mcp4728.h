#ifndef _MCP4728_H_
#define _MCP4728_H_
#include "hardware/i2c/i2cdevice.h"
#include "hardware/device_types.h"
#include <chrono>

/* MCP4728 4ch 12bit DAC */

class MCP4728 : public i2cDevice, public DeviceFunction<DeviceType::DAC>, public static_device_base<MCP4728> {
    // the DAC supports writing to input register but not sending latch bit to update the output register
    // here we will always send the "UDAC" (latch) bit because we don't need this functionality
    // MCP4728 listens to I2C Generall Call Commands
    // reset, wake-up, software update, read address bits
    // reset is "0x00 0x06"
    // wake-up is "0x00 0x09"
public:
    enum CFG_GAIN { GAIN1 = 0,
        GAIN2 = 1 };
    enum CFG_VREF { VREF_VDD = 0,
        VREF_2V = 1 };

    // struct that characterizes one dac output channel
    // setting the eeprom flag enables access to the eeprom registers instead of the dac output registers
    struct DacChannel {
        uint8_t pd = 0x00;
        CFG_GAIN gain = GAIN1;
        CFG_VREF vref = VREF_2V;
        bool eeprom = false;
        uint16_t value = 0;
    };

    MCP4728()
        : i2cDevice(0x60)
    {
        fTitle = fName = "MCP4728";
    }
    MCP4728(const char* busAddress, uint8_t slaveAddress)
        : i2cDevice(busAddress, slaveAddress)
    {
        fTitle = fName = "MCP4728";
    }
    MCP4728(uint8_t slaveAddress)
        : i2cDevice(slaveAddress)
    {
        fTitle = fName = "MCP4728";
    }
    bool devicePresent() override;
	bool setVoltage( unsigned int channel, float voltage ) override;
	bool storeSettings() override;
	bool writeChannel( uint8_t channel, const DacChannel& channelData );
	bool readChannel( uint8_t channel, DacChannel& channelData );
	bool setVRef( unsigned int channel, CFG_VREF vref_setting );
	bool setVRef( CFG_VREF vref_setting );
	
    static float code2voltage(const DacChannel& channelData);

	bool identify() override;
	bool probeDevicePresence() override { return devicePresent(); }

private:
	static constexpr float fVddRefVoltage { 3.3 }; ///< voltage at which the device is powered
	enum COMMAND: uint8_t {
		DAC_FAST_WRITE		= 	0b00000000,
		DAC_MULTI_WRITE		=	0b00001000,
		DAC_EEP_SEQ_WRITE	=	0b00001010,
		DAC_EEP_SINGLE_WRITE=	0b00001011,
		ADDR_BITS_WRITE		=	0b00001100,
		VREF_WRITE			= 	0b00010000,
		GAIN_WRITE			=	0b00011000,
		PD_WRITE			=	0b00010100
	};
	DacChannel fChannelSetting[4], fChannelSettingEep[4];
	std::chrono::time_point<std::chrono::steady_clock> fLastRegisterUpdate { };
	
	bool setVoltage( uint8_t channel, float voltage, bool toEEPROM );
	bool setValue( uint8_t channel, uint16_t value, CFG_GAIN gain = GAIN1, bool toEEPROM = false );
	bool readRegisters();
	void parseChannelData( uint8_t* buf );
	void dumpRegisters();
};

#endif //!_MCP4728_H_
