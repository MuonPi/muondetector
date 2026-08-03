#ifndef _QMC5883_H_
#define _QMC5883_H_

#include "device_types.h"
#include "i2c/i2cdevice.h"

#include <cstdint>

class QMC5883 : public i2cDevice,
                public DeviceFunction<DeviceType::MAGNETIC_FIELD>,
                public static_device_base<QMC5883> {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS{0x0d};
    static constexpr std::uint8_t EXPECTED_CHIP_ID{0xff};

    enum class MODE : std::uint8_t {
        STANDBY = 0x00,
        CONTINUOUS = 0x01,
    };

    enum class OUTPUT_DATA_RATE : std::uint8_t {
        HZ_10 = 0x00,
        HZ_50 = 0x01,
        HZ_100 = 0x02,
        HZ_200 = 0x03,
    };

    enum class RANGE : std::uint8_t {
        G_2 = 0x00,
        G_8 = 0x01,
    };

    enum class OVER_SAMPLE_RATIO : std::uint8_t {
        OSR_512 = 0x00,
        OSR_256 = 0x01,
        OSR_128 = 0x02,
        OSR_64 = 0x03,
    };

    struct Vector3 {
        double x{0.0};
        double y{0.0};
        double z{0.0};
    };

    struct RawReading {
        std::int16_t x{0};
        std::int16_t y{0};
        std::int16_t z{0};
        bool valid{false};
    };

    QMC5883();
    QMC5883(const char* busAddress, std::uint8_t slaveAddress);
    QMC5883(std::uint8_t slaveAddress);
    virtual ~QMC5883();

    bool init(MODE mode = MODE::CONTINUOUS,
              OUTPUT_DATA_RATE outputDataRate = OUTPUT_DATA_RATE::HZ_10, RANGE range = RANGE::G_2,
              OVER_SAMPLE_RATIO overSampleRatio = OVER_SAMPLE_RATIO::OSR_512);
    bool reset();
    bool setConfig(MODE mode, OUTPUT_DATA_RATE outputDataRate, RANGE range,
                   OVER_SAMPLE_RATIO overSampleRatio);
    bool setMode(MODE mode);
    bool setOutputDataRate(OUTPUT_DATA_RATE outputDataRate);
    bool setRange(RANGE range);
    bool setOverSampleRatio(OVER_SAMPLE_RATIO overSampleRatio);
    bool setResetPeriod(std::uint8_t period = 0x01);

    std::uint8_t chipId();
    std::uint8_t status();
    bool dataReady();
    bool dataOverflow();
    bool dataSkipped();
    bool waitDataReady(unsigned int timeoutMillis = 200);

    RawReading readRaw();
    bool readRaw(RawReading& reading);
    Vector3 readMagneticField();
    bool readMagneticField(Vector3& magneticFieldGauss);
    double readMagnitude();
    bool readMagnitude(double& magnitudeGauss);
    bool readTemperatureRaw(std::int16_t& temperature);
    bool readTemperature(double& temperature);

    bool getMagneticFieldRawValueXYZ(std::int16_t* value);
    bool getMagneticFieldXYZ(double* magnet);
    bool getMagneticField(double& magnet);
    bool getTemperatureRawValue(std::int16_t& temperature);
    bool getTemperature(double& temperature);

    double fullScaleRangeGauss() const;
    MODE mode() const { return fModeSetting; }
    OUTPUT_DATA_RATE outputDataRate() const { return fOutputDataRate; }
    RANGE range() const { return fRange; }
    OVER_SAMPLE_RATIO overSampleRatio() const { return fOverSampleRatio; }

    bool identify() override;
    bool probeDevicePresence() override { return devicePresent(); }
    bool devicePresent() override;

  private:
    enum REG : std::uint8_t {
        DATA_X_LSB = 0x00,
        STATUS = 0x06,
        TEMP_LSB = 0x07,
        CONTROL_1 = 0x09,
        CONTROL_2 = 0x0a,
        SET_RESET_PERIOD = 0x0b,
        CHIP_ID = 0x0d
    };

    bool writeRegister(REG reg, std::uint8_t value);
    bool writeControl1();
    static std::int16_t decodeInt16(std::uint8_t lowByte, std::uint8_t highByte);

    MODE fModeSetting{MODE::STANDBY};
    OUTPUT_DATA_RATE fOutputDataRate{OUTPUT_DATA_RATE::HZ_10};
    RANGE fRange{RANGE::G_2};
    OVER_SAMPLE_RATIO fOverSampleRatio{OVER_SAMPLE_RATIO::OSR_512};
};

#endif // _QMC5883_H_
