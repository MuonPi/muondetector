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

bool MCP4728::setVoltage(unsigned int channel, float voltage)
{
	if ( voltage < 0 || channel > 3 ) {
		return false;
	}
	return setVoltage( channel, voltage, false );
}

bool MCP4728::setVoltage(uint8_t channel, float voltage, bool toEEPROM)
{
    if ( voltage < 0 || channel > 3 ) {
        return false;
    }
    CFG_GAIN gain = GAIN1;
    // Vref=internal: Vout = (2.048V * Dn) / 4096 * Gx
    // Vref=Vdd: Vout = (Vdd * Dn) / 4096
    uint16_t value { 0x0000 };
	if ( fChannelSetting[channel].vref == VREF_2V ) {
		value = std::lround( voltage * 2000 );
		if ( value > 0xfff ) {
			value = value >> 1;
			gain = GAIN2;
		}
	} else {
		value = std::lround( voltage * 4096 / fVddRefVoltage );
	}

	if (value > 0xfff) {
        // error: desired voltage is out of range
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
	if ( !waitEepReady() ) return false;

    channel = channel & 0x03;
    uint8_t buf[3];
    if (channelData.eeprom) {
        buf[0] = COMMAND::DAC_EEP_SINGLE_WRITE << 3;
		//buf[0] = 0b01011001;
    } else {
        buf[0] = COMMAND::DAC_MULTI_WRITE << 3;
		//buf[0] = 0b01000001;
    }
    // buf[0] |= 0x01; set UDAC bit
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
    // force a register update next time anything is read
    fLastRegisterUpdate = { };
    return true;
}

bool MCP4728::storeSettings()
{
    startTimer();
    const uint8_t startchannel { 0 };
    uint8_t buf[9];
	
	if ( !waitEepReady() ) return false;
	
	buf[0] = COMMAND::DAC_EEP_SEQ_WRITE << 3;
    //buf[0] |= 0x01; // set UDAC bit
    buf[0] |= ( startchannel << 1 ); // command DAC1 DAC0 UDAC
    for ( uint8_t channel = 0; channel < 4; channel++) {
		buf[ channel*2+1 ] = ((uint8_t)fChannelSetting[channel].vref) << 7; // Vref PD1 PD0 Gx (gain) D11 D10 D9 D8
		buf[ channel*2+1 ] |= (fChannelSetting[channel].pd & 0x03) << 5;
		buf[ channel*2+1 ] |= (uint8_t)((fChannelSetting[channel].value & 0xf00) >> 8);
		buf[ channel*2+1 ] |= (uint8_t)(fChannelSetting[channel].gain & 0x01) << 4;
		buf[ channel*2+2 ] = (uint8_t)(fChannelSetting[channel].value & 0xff); // D7 D6 D5 D4 D3 D2 D1 D0
	}
	
	if ( write(buf, 9) != 9 ) {
        // somehow did not write exact same amount of bytes as it should
        return false;
    }
    stopTimer();
    fLastConvTime = fLastTimeInterval;
    // force a register update next time anything is read
    fLastRegisterUpdate = { };
	
	return true;
}

bool MCP4728::devicePresent()
{
	return readRegisters();
}

bool MCP4728::waitEepReady()
{
//	if ( !fBusy ) return false;
	if ( !readRegisters() ) return false;
	int timeout_ctr { 100 };
	while ( fBusy && timeout_ctr-- > 0 ) {
		fLastRegisterUpdate = {};
		if ( !readRegisters() ) return false;
	}
//	std::cout<<"debug: MCP4728::waitEepReady(): timout_ctr="<<std::dec<<timeout_ctr<<std::endl;
	if ( timeout_ctr > 0 ) return true;
	return false;
}

bool MCP4728::readRegisters()
{
    uint8_t buf[24];
    startTimer();
	// perform a register update only if the buffered content is too old
	if ( ( std::chrono::steady_clock::now() - fLastRegisterUpdate ) < DataValidityTimeout ) {
//		std::cout<<std::dec<<"debug: data age="<<(std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::steady_clock::now()-fLastRegisterUpdate ).count())<<"ms"<<std::endl;
		return true;
	}
    // perform a read sequence of all registers as described in datasheet
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
	startTimer();
	if ( !waitEepReady() ) return false;

	channel = channel & 0x03;
	uint8_t databyte { 0 };
	databyte = COMMAND::VREF_WRITE >> 1;
	for ( unsigned int ch = 0; ch < 4; ch++ ) {
		if ( ch == channel ) {
			databyte |= vref_setting;
		} else {
			databyte |= fChannelSetting[ch].vref;
		}
		databyte = (databyte << 1);
	}
	
    if ( write( &databyte, 1 ) != 1 ) {
        // somehow did not write exact same amount of bytes as it should
        return false;
    }
    stopTimer();
    fLastConvTime = fLastTimeInterval;
	
	fChannelSetting[channel].vref = vref_setting;
    return true;
}

bool MCP4728::setVRef( CFG_VREF vref_setting )
{
	startTimer();
	if ( !waitEepReady() ) return false;
	uint8_t databyte { 0 };
	databyte = COMMAND::VREF_WRITE >> 1;
	for ( unsigned int ch = 0; ch < 4; ch++ ) {
		databyte |= vref_setting;
		databyte = (databyte << 1);
	}
	
    if ( write( &databyte, 1 ) != 1 ) {
        // somehow did not write exact same amount of bytes as it should
        return false;
    }
    stopTimer();
    fLastConvTime = fLastTimeInterval;
	
	fChannelSetting[0].vref = fChannelSetting[1].vref = fChannelSetting[2].vref = fChannelSetting[3].vref = vref_setting;
    return true;
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
		
		fBusy = ( buf[21] & 0x80 ) == 0;
		
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
