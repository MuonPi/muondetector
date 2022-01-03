#ifndef _DEVICE_TYPES_H_
#define _DEVICE_TYPES_H_

#include <map>
#include <string>
#include <chrono>
#include <functional>

enum class DeviceType {
	ADC, DAC, IO_EXTENDER, TEMP, PRESSURE, HUMIDITY, EEPROM, BUS_SWITCH, RTC, TDC, DISPLAY, MAGNETIC_FIELD, GYRO, ACCELERATION, OTHER
};

const std::map<DeviceType, std::string> DeviceTypes = { 
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
	{ DeviceType::MAGNETIC_FIELD, "MAGNETIC_FIELD" },
	{ DeviceType::GYRO, "GYRO" },
	{ DeviceType::ACCELERATION, "ACCELERATION" },
	{ DeviceType::OTHER, "OTHER" }
};


template <DeviceType T = DeviceType::OTHER>
class DeviceFunctionBase {
public:
	DeviceType type { T };
	const std::string& getName() { return fName; }
	void setName( const std::string& name ) { fName = name; }
	static const std::string typeString() {
		auto it = DeviceTypes.find( T );
		if ( it != DeviceTypes.end() ) return it->second;
		return "UNKNOWN";
	}
	/** @brief probe the presence of the device
	 * This method provides a functionality to find out wether the given device is responsive. 
	 * @return Device was responding (true = success)
	 * @note The probing should be non-invasive, i.e. most devices will not change settings when a read access is performed.
	 * This method must be implemented in the derived classes.
	 * */
	virtual bool probeDevicePresence() = 0;
protected:
	std::string fName { DeviceTypes.at(T)+"_DEVICE" };
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
 the specialization has getVoltage(), getSample() and triggerConversion() methods, which must be implemented in the derived class
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

/** specialization for DACs
 the specialization has a setVoltage() method, which must be implemented in the derived class
*/
template <>
class DeviceFunction<DeviceType::DAC> : public DeviceFunctionBase<DeviceType::DAC> {
public:
	virtual bool setVoltage( unsigned int channel, float voltage ) = 0;
	virtual bool storeSettings() = 0;
	double getLastConvTime() const { return fLastConvTime; }
protected:
	double fLastConvTime { 0. };
};

/** specialization for EEPROMs
 the specialization has readBytes() and writeBytes methods, which must be implemented in the derived class
*/
template <>
class DeviceFunction<DeviceType::EEPROM> : public DeviceFunctionBase<DeviceType::EEPROM> {
public:
	virtual int16_t readBytes(uint8_t regAddr, uint16_t length, uint8_t* data) = 0;
	virtual bool writeBytes(uint8_t addr, uint16_t length, uint8_t* data) = 0;
protected:
};

/** specialization for I/O extenders
 the specialization has setOutputPorts(), setOutputState and getInputState methods, which must be implemented in the derived class
*/
template <>
class DeviceFunction<DeviceType::IO_EXTENDER> : public DeviceFunctionBase<DeviceType::IO_EXTENDER> {
public:
	virtual bool setOutputPorts(uint8_t portMask) = 0;
	virtual bool setOutputState(uint8_t portMask) = 0;
	virtual uint8_t getInputState() = 0;
protected:
};

#endif // !_DEVICE_TYPES_H_