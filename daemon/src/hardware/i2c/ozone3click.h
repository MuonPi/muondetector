#ifndef _OZONE3CLICK_H_
#define _OZONE3CLICK_H_

#include "device_types.h"
#include "i2c/i2cdevice.h"

#include <chrono>
#include <cstdint>

class Ozone3Click : public i2cDevice,
                    public DeviceFunction<DeviceType::ADC>,
                    public static_device_base<Ozone3Click> {
  public:
    static constexpr std::uint8_t DEFAULT_LMP91000_ADDRESS{0x48};
    static constexpr std::uint8_t DEFAULT_ADC_ADDRESS{0x4d};
    static constexpr std::uint8_t DEFAULT_TIA_CONTROL{0x1f};
    static constexpr std::uint8_t DEFAULT_REFERENCE_CONTROL{0xa0};
    static constexpr std::uint16_t ADC_FULL_SCALE{0x0fff};
    static constexpr double ADC_CODE_COUNT{4096.0};
    static constexpr double DEFAULT_ADC_REFERENCE_VOLTAGE{3.3};
    static constexpr double DEFAULT_LMP_REFERENCE_VOLTAGE{2.048};
    static constexpr double DEFAULT_OZONE_SENSITIVITY_NA_PER_PPM{-60.0};
    static constexpr double DEFAULT_ZERO_OFFSET_VOLTAGE{0.0};

    enum class Register : std::uint8_t {
        STATUS = 0x00,
        LOCK = 0x01,
        TIACN = 0x10,
        REFCN = 0x11,
        MODECN = 0x12
    };

    enum class Mode : std::uint8_t {
        DEEP_SLEEP = 0x00,
        TWO_LEAD = 0x01,
        STANDBY = 0x02,
        THREE_LEAD = 0x03,
        TEMPERATURE_TIA_OFF = 0x06,
        TEMPERATURE_TIA_ON = 0x07
    };

    struct Config {
        constexpr Config(std::uint8_t tiaControlValue = DEFAULT_TIA_CONTROL,
                         std::uint8_t referenceControlValue = DEFAULT_REFERENCE_CONTROL,
                         Mode modeValue = Mode::THREE_LEAD)
            : tiaControl(tiaControlValue)
            , referenceControl(referenceControlValue)
            , mode(modeValue) {}

        std::uint8_t tiaControl;       // 350k internal RTIA, 100 ohm RLOAD
        std::uint8_t referenceControl; // external VREF, 50% zero, 0% negative bias
        Mode mode;
    };

    struct Reading {
        std::chrono::time_point<std::chrono::steady_clock> timestamp;
        std::uint16_t rawAdc{0};
        double adcVoltage{0.0};
        double lmpZeroVoltage{0.0};
        double sensorCurrentNanoampere{0.0};
        double ozonePpbv{0.0};
        double mikroEExamplePpm{0.0};
    };

    Ozone3Click();
    explicit Ozone3Click(std::uint8_t lmpAddress);
    Ozone3Click(std::uint8_t lmpAddress, std::uint8_t adcAddress);
    Ozone3Click(const char* busAddress, std::uint8_t lmpAddress,
                std::uint8_t adcAddress = DEFAULT_ADC_ADDRESS,
                double adcReferenceVoltage = DEFAULT_ADC_REFERENCE_VOLTAGE,
                double lmpReferenceVoltage = DEFAULT_LMP_REFERENCE_VOLTAGE,
                double ozoneSensitivityNanoamperePerPpm = DEFAULT_OZONE_SENSITIVITY_NA_PER_PPM,
                double zeroOffsetVoltage = DEFAULT_ZERO_OFFSET_VOLTAGE);
    virtual ~Ozone3Click();

    bool init(const Config& config = Config{});
    bool configure(const Config& config = Config{});
    bool identify() override;
    bool devicePresent() override;
    bool probeDevicePresence() override { return devicePresent(); }

    bool readLmpRegister(Register reg, std::uint8_t& value);
    bool writeLmpRegister(Register reg, std::uint8_t value);
    bool waitReady(unsigned int timeoutMs = 100);
    bool readAdcRaw(std::uint16_t& raw, std::uint8_t* upperByte = nullptr,
                    std::uint8_t* lowerByte = nullptr);
    bool readMeasurement(Reading& reading);

    double rawToVoltage(std::uint16_t raw) const;
    double lmpZeroVoltage() const;
    double rawToSensorCurrentNanoampere(std::uint16_t raw) const;
    double rawToOzonePpbv(std::uint16_t raw) const;
    static double internalZeroFraction(std::uint8_t referenceControl);
    static double tiaResistanceOhm(std::uint8_t tiaControl);
    static double rawToMikroEExamplePpm(std::uint16_t raw);

    double getVoltage(unsigned int channel = 0) override;
    Sample getSample(unsigned int channel = 0) override;
    bool triggerConversion(unsigned int channel) override;

    bool getOzoneRawValue(std::int16_t& ozone);
    bool getOzonePpbv(double& ozonePpbv);
    bool getOzonePpm(double& ozonePpm);
    bool getOzone(double& ozone);

    std::uint8_t lmpAddress() const { return fLmpAddress; }
    std::uint8_t adcAddress() const { return fAdcAddress; }
    double adcReferenceVoltage() const { return fAdcReferenceVoltage; }
    double lmpReferenceVoltage() const { return fLmpReferenceVoltage; }
    double ozoneSensitivityNanoamperePerPpm() const { return fOzoneSensitivityNanoamperePerPpm; }
    double zeroOffsetVoltage() const { return fZeroOffsetVoltage; }
    Config config() const { return fConfig; }

  private:
    void setLmpAddress();
    void setAdcAddress();

    std::uint8_t fLmpAddress{DEFAULT_LMP91000_ADDRESS};
    std::uint8_t fAdcAddress{DEFAULT_ADC_ADDRESS};
    double fAdcReferenceVoltage{DEFAULT_ADC_REFERENCE_VOLTAGE};
    double fLmpReferenceVoltage{DEFAULT_LMP_REFERENCE_VOLTAGE};
    double fOzoneSensitivityNanoamperePerPpm{DEFAULT_OZONE_SENSITIVITY_NA_PER_PPM};
    double fZeroOffsetVoltage{DEFAULT_ZERO_OFFSET_VOLTAGE};
    Config fConfig{};
};

using OZONE3CLICK = Ozone3Click;

#endif // _OZONE3CLICK_H_
