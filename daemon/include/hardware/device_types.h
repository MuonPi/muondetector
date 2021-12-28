#ifndef _DEVICE_TYPES_H_
#define _DEVICE_TYPES_H_
//#include "hardware/i2c/i2cdevice.h"
#include <map>
#include <string>
#include <chrono>
#include <functional>

enum class DeviceType {
	ADC, DAC, IO_EXTENDER, TEMP, PRESSURE, HUMIDITY, EEPROM, BUS_SWITCH, RTC, TDC, DISPLAY, OTHER
};

const std::map<DeviceType, std::string> DeviceTypeStrings = { 
	{ DeviceType::ADC, "ADC"},
	{ DeviceType::DAC, "DAC" },
	{ DeviceType::IO_EXTENDER, "IO_EXTENDER" },
	{ DeviceType::TEMP, "TEMP" },
	{ DeviceType::PRESSURE, "PRESSURE" },
	{ DeviceType::HUMIDITY, "HUMIDITY" },
	{ DeviceType::EEPROM, "EEPROM" },
	{ DeviceType::BUS_SWITCH, "BUS_SWITCH" },
	{ DeviceType::RTC, "RTC" },
	{ DeviceType::TDC, "TDC" },
	{ DeviceType::DISPLAY, "DISPLAY" },
	{ DeviceType::OTHER, "OTHER" }
};


template <DeviceType T = DeviceType::OTHER>
class DeviceFunctionBase {
public:
	DeviceType type { T };
	static const std::string typeString() {
		auto it = DeviceTypeStrings.find( T );
		if ( it != DeviceTypeStrings.end() ) return it->second;
		return "UNKNOWN";
	}
};

/// abstract empty general template class for device functions
/// all methods which are specific for a device function shall be declared in the template specializations of this class
template <DeviceType T>
class DeviceFunction : public DeviceFunctionBase<T> {
};


//
// here follow the specializations for specific device functions
//

/** specialization for temperature sensor devices
 the specialization has a getTemperature() method, which must be implemented in the derived class
*/
template <>
class DeviceFunction<DeviceType::TEMP> : public DeviceFunctionBase<DeviceType::TEMP> {
public:
	virtual float getTemperature() = 0;
	float lastTemperatureValue() const { return fLastTemp; }
protected:
	float fLastTemp;
};

/** specialization for ADCs
 the specialization has getVoltage() and getSample methods, which must be implemented in the derived class
*/
template <>
class DeviceFunction<DeviceType::ADC> : public DeviceFunctionBase<DeviceType::ADC> {
public:
	struct Sample {
		std::chrono::time_point<std::chrono::steady_clock> timestamp;
		int value;
		float voltage;
		float lsb_voltage;
		unsigned int channel;
		bool operator==(const Sample& other);
		bool operator!=(const Sample& other);
	};
	static constexpr Sample InvalidSample { std::chrono::steady_clock::time_point::min(), 0, 0., 0., 0 };

	virtual double getVoltage( unsigned int channel = 0 ) = 0;
	virtual Sample getSample( unsigned int channel = 0 ) = 0;
	virtual bool triggerConversion( unsigned int channel ) = 0;
	void registerConversionReadyCallback(std::function<void(Sample)> fn) {	fConvReadyFn = fn; }
	double getLastConvTime() const { return fLastConvTime; }
protected:
	std::function<void(Sample)> fConvReadyFn { };
	double fLastConvTime { 0. };
};

#endif // !_DEVICE_TYPES_H_