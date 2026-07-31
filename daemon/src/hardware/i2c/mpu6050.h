#ifndef _MPU6050_H_
#define _MPU6050_H_

#include "device_types.h"
#include "i2c/i2cdevice.h"

#include <cstdint>

class MPU6050 : public i2cDevice,
                public DeviceFunction<DeviceType::OTHER>,
                public static_device_base<MPU6050> {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS{0x68};
    static constexpr std::uint8_t ALTERNATE_ADDRESS{0x69};
    static constexpr std::uint8_t EXPECTED_WHO_AM_I{0x68};

    enum class REG : std::uint8_t {
        SMPLRT_DIV = 0x19,
        CONFIG = 0x1a,
        GYRO_CONFIG = 0x1b,
        ACCEL_CONFIG = 0x1c,
        ACCEL_XOUT_H = 0x3b,
        TEMP_OUT_H = 0x41,
        GYRO_XOUT_H = 0x43,
        PWR_MGMT_1 = 0x6b,
        PWR_MGMT_2 = 0x6c,
        WHO_AM_I = 0x75
    };

    enum class CLOCK_SOURCE : std::uint8_t {
        INTERNAL = 0x00,
        PLL_X_GYRO = 0x01,
        PLL_Y_GYRO = 0x02,
        PLL_Z_GYRO = 0x03,
        PLL_EXTERNAL_32KHZ = 0x04,
        PLL_EXTERNAL_19MHZ = 0x05,
        STOP = 0x07
    };

    enum class GYRO_RANGE : std::uint8_t {
        DPS_250 = 0x00,
        DPS_500 = 0x01,
        DPS_1000 = 0x02,
        DPS_2000 = 0x03
    };

    enum class ACCEL_RANGE : std::uint8_t { G_2 = 0x00, G_4 = 0x01, G_8 = 0x02, G_16 = 0x03 };

    enum class DLPF : std::uint8_t {
        ACCEL_260HZ_GYRO_256HZ = 0x00,
        ACCEL_184HZ_GYRO_188HZ = 0x01,
        ACCEL_94HZ_GYRO_98HZ = 0x02,
        ACCEL_44HZ_GYRO_42HZ = 0x03,
        ACCEL_21HZ_GYRO_20HZ = 0x04,
        ACCEL_10HZ_GYRO_10HZ = 0x05,
        ACCEL_5HZ_GYRO_5HZ = 0x06
    };

    struct Vector3 {
        double x{0.0};
        double y{0.0};
        double z{0.0};
    };

    struct RawReading {
        std::int16_t accelX{0};
        std::int16_t accelY{0};
        std::int16_t accelZ{0};
        std::int16_t temperature{0};
        std::int16_t gyroX{0};
        std::int16_t gyroY{0};
        std::int16_t gyroZ{0};
        bool valid{false};
    };

    struct Measurement {
        Vector3 accelerationG{};
        Vector3 gyroscopeDps{};
        double temperatureC{-999.0};
        bool valid{false};
    };

    MPU6050();
    MPU6050(const char* busAddress, std::uint8_t slaveAddress);
    MPU6050(std::uint8_t slaveAddress);
    virtual ~MPU6050();

    bool init(GYRO_RANGE gyroRange = GYRO_RANGE::DPS_250, ACCEL_RANGE accelRange = ACCEL_RANGE::G_2,
              DLPF dlpf = DLPF::ACCEL_44HZ_GYRO_42HZ, std::uint8_t sampleRateDivider = 0);
    bool reset();
    bool wake(CLOCK_SOURCE clockSource = CLOCK_SOURCE::PLL_X_GYRO);
    bool sleep(bool enabled = true);

    bool setSampleRateDivider(std::uint8_t divider);
    bool setDigitalLowPassFilter(DLPF dlpf);
    bool setGyroRange(GYRO_RANGE range);
    bool setAccelRange(ACCEL_RANGE range);
    bool readGyroRange(GYRO_RANGE& range);
    bool readAccelRange(ACCEL_RANGE& range);

    std::uint8_t whoAmI();
    bool identify() override;
    bool devicePresent() override;
    bool probeDevicePresence() override { return devicePresent(); }

    RawReading readRaw();
    bool readRaw(RawReading& reading);
    Measurement getMeasurement();
    bool getMeasurement(Measurement& measurement);
    bool getAcceleration(Vector3& accelerationG);
    bool getGyroscope(Vector3& gyroscopeDps);
    bool getTemperature(double& temperatureC);
    double getTemperature();

    GYRO_RANGE gyroRange() const { return fGyroRange; }
    ACCEL_RANGE accelRange() const { return fAccelRange; }
    DLPF digitalLowPassFilter() const { return fDlpf; }
    std::uint8_t sampleRateDivider() const { return fSampleRateDivider; }
    RawReading lastRawReading() const { return fLastRawReading; }
    Measurement lastMeasurement() const { return fLastMeasurement; }

    double accelerationSensitivity() const;
    double gyroscopeSensitivity() const;

    static bool isValidAddress(std::uint8_t address);
    static bool isExpectedWhoAmI(std::uint8_t deviceId);

  private:
    bool updateRegister(REG reg, std::uint8_t mask, std::uint8_t value);
    bool readScaledVector(REG reg, double sensitivity, Vector3& vector);
    Vector3 scaleAcceleration(const RawReading& reading) const;
    Vector3 scaleGyroscope(const RawReading& reading) const;
    static std::int16_t decodeInt16(std::uint8_t highByte, std::uint8_t lowByte);

    GYRO_RANGE fGyroRange{GYRO_RANGE::DPS_250};
    ACCEL_RANGE fAccelRange{ACCEL_RANGE::G_2};
    DLPF fDlpf{DLPF::ACCEL_260HZ_GYRO_256HZ};
    std::uint8_t fSampleRateDivider{0};
    RawReading fLastRawReading{};
    Measurement fLastMeasurement{};
};

using GY521 = MPU6050;

#endif // !_MPU6050_H_
