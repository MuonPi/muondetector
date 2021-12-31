#include "hardware/i2c/mcp4728.h"
#include <stdint.h>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cmath>

/*
* MCP4728 4 ch 12 bit DAC
*/
constexpr auto DataValidityTimeout { std::chrono::milliseconds( 100 ) };

bool MCP4728::setVoltage(uint8_t channel, float voltage, bool toEEPROM)
{
    if (voltage < 0) {
        return false;
    }
    CFG_GAIN gain = GAIN1;
    // Vout = (2.048V * Dn) / 4096 * Gx <= VDD
    unsigned int value = std::lround( voltage * 2000 );
    if (value > 0xfff) {
        value = value >> 1;
        gain = GAIN2;
    }
    if (value > 0xfff) {
        // error message
        return false;
    }
    return setValue(channel, value, gain, toEEPROM);
}

bool MCP4728::setValue(uint8_t channel, uint16_t value, CFG_GAIN gain, bool toEEPROM)
{
    if (value > 0xfff) {
        value = 0xfff;
        // error: number of bits exceeding 12
        return false;
    }
    DacChannel dacChannel { fChannelSetting[channel] };
    if ( toEEPROM ) {
		dacChannel = fChannelSettingEep[channel];
		dacChannel.eeprom = true;
	}
	dacChannel.value = value;
	dacChannel.gain = gain;
    
	return writeChannel( channel, dacChannel );
}

bool MCP4728::writeChannel(uint8_t channel, const DacChannel& channelData)
{
    if (channelData.value > 0xfff) {
        // error number of bits exceeding 12
        return false;
    }
    startTimer();
    channel = channel & 0x03;
    uint8_t buf[3];
    if (channelData.eeprom) {
        buf[0] = COMMAND::DAC_EEP_SINGLE_WRITE << 3;
		//buf[0] = 0b01011001;
    } else {
        buf[0] = COMMAND::DAC_MULTI_WRITE << 3;
		//buf[0] = 0b01000001;
    }
    buf[0] |= (channel << 1); // 01000/01011 (multiwrite/singlewrite command) DAC1 DAC0 (channel) UDAC bit = 1
    buf[1] = ((uint8_t)channelData.vref) << 7; // Vref PD1 PD0 Gx (gain) D11 D10 D9 D8
    buf[1] |= (channelData.pd & 0x03) << 5;
    buf[1] |= (uint8_t)((channelData.value & 0xf00) >> 8);
    buf[1] |= (uint8_t)(channelData.gain & 0x01) << 4;
    buf[2] = (uint8_t)(channelData.value & 0xff); // D7 D6 D5 D4 D3 D2 D1 D0
    if ( write(buf, 3) != 3 ) {
        // somehow did not write exact same amount of bytes as it should
        return false;
    }
    stopTimer();
    fLastConvTime = fLastTimeInterval;
	if ( channelData.eeprom ) {
		fChannelSettingEep[channel] = channelData;
	} else {
		fChannelSetting[channel] = channelData;
	}
    return true;
}

bool MCP4728::storeSettings()
{
	return false;
}

bool MCP4728::devicePresent()
{
	return readRegisters();
}

bool MCP4728::readRegisters()
{
    uint8_t buf[24];
    startTimer();
    // perform a read sequence of all registers as described in datasheet
    if ( ( std::chrono::steady_clock::now() - fLastRegisterUpdate ) < DataValidityTimeout ) return true;
	if ( 24 != read(buf, 24) ) return false;
    stopTimer();
    parseChannelData( buf );
	fLastRegisterUpdate = std::chrono::steady_clock::now();
	return true;
}

bool MCP4728::readChannel(uint8_t channel, DacChannel& channelData)
{
    if (channel > 3) {
        // error: channel index exceeding 3
        return false;
    }

	if ( !readRegisters() ) return false;
	if ( channelData.eeprom ) {
		channelData = fChannelSettingEep[channel];
	} else {
		channelData = fChannelSetting[channel];
	}

    return true;
}

float MCP4728::code2voltage(const DacChannel& channelData)
{
    float vref = (channelData.vref == VREF_2V) ? 2.048 : fVddRefVoltage;
    float voltage = vref * channelData.value / 4096.;
    if (channelData.gain == GAIN2 && channelData.vref != VREF_VDD)
        voltage *= 2.;
    return voltage;
}

bool MCP4728::identify() {
	if ( fMode == MODE_FAILED ) return false;
	if ( !devicePresent() ) return false;
    uint8_t buf[24];
    if (read(buf, 24) != 24) return false;
	
	if ( ( ( buf[0] & 0xf0 ) == 0xc0 ) &&
		 ( ( buf[6] & 0xf0 ) == 0xd0 ) &&
		 ( ( buf[12] & 0xf0 ) == 0xe0 ) &&
		 ( ( buf[18] & 0xf0 ) == 0xf0 ) )
	{
		return true;
	}
	
	return false;
}

bool MCP4728::setVRef( unsigned int channel, CFG_VREF vref_setting )
{
	return false;
}

void MCP4728::parseChannelData( uint8_t* buf )
{
	for ( unsigned int channel = 0; channel < 4; channel++ ) {
		// dac reg: offs = 1
		// eep: offs = 4
		fChannelSetting[channel].vref = (buf[channel * 6 + 1] & 0x80) ? VREF_2V : VREF_VDD;
		fChannelSettingEep[channel].vref = (buf[channel * 6 + 4] & 0x80) ? VREF_2V : VREF_VDD;

		fChannelSetting[channel].pd = (buf[channel * 6 + 1] & 0x60) >> 5;
		fChannelSettingEep[channel].pd = (buf[channel * 6 + 4] & 0x60) >> 5;

		fChannelSetting[channel].gain = (buf[channel * 6 + 1] & 0x10) ? GAIN2 : GAIN1;
		fChannelSettingEep[channel].gain = (buf[channel * 6 + 4] & 0x10) ? GAIN2 : GAIN1;
		
		fChannelSetting[channel].value = (uint16_t)(buf[channel * 6 + 1] & 0x0f) << 8;
		fChannelSetting[channel].value |= (uint16_t)(buf[channel * 6 + 1 + 1] & 0xff);
		
		fChannelSettingEep[channel].value = (uint16_t)(buf[channel * 6 + 4] & 0x0f) << 8;
		fChannelSettingEep[channel].value |= (uint16_t)(buf[channel * 6 + 4 + 1] & 0xff);
		
		fChannelSettingEep[channel].eeprom = true;
	}
}

void MCP4728::dumpRegisters() {
    uint8_t buf[24];
    if (read(buf, 24) != 24) {
        // somehow did not read exact same amount of bytes as it should
        return;
    }
	for ( int ch = 0; ch < 4; ch++ ) {
		std::cout << "DAC"<<ch<<": " << std::setw(2) << std::setfill('0') << std::hex 
		<< (int)buf[ch*6] << " " << (int)buf[ch*6+1] << " " << (int)buf[ch*6+2]
		<< " (eep: " << (int)buf[ch*6+3] << " " << (int)buf[ch*6+4] << " " << (int)buf[ch*6+5] << ")\n";
	}
}
