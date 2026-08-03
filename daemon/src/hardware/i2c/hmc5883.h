#ifndef _HMC5883_H_
#define _HMC5883_H_

#include "device_types.h"
#include "i2c/i2cdevice.h"

#include <cstdint>

/* HMC5883  */

class HMC5883 : public i2cDevice,
                public DeviceFunction<DeviceType::MAGNETIC_FIELD>,
                public static_device_base<HMC5883> {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS{0x1e};

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

    // Resolution for the 8 gain settings in mG/LSB
    static const double GAIN[8];
    HMC5883();
    HMC5883(const char* busAddress, uint8_t slaveAddress);
    HMC5883(uint8_t slaveAddress);
    virtual ~HMC5883();

    bool init();
    // gain range 0..7
    bool setGain(uint8_t gain);
    uint8_t readGain();
    RawReading readRaw();
    bool readRaw(RawReading& reading);
    Vector3 readMagneticField();
    bool readMagneticField(Vector3& magneticFieldGauss);
    bool getXYZRawValues(int& x, int& y, int& z);
    bool getXYZMagneticFields(double& x, double& y, double& z);
    bool readRDYBit();
    bool readLockBit();
    bool waitDataReady(unsigned int timeoutMillis = 100);
    bool calibrate(int& x, int& y, int& z);
    bool identify() override;
    bool probeDevicePresence() override { return devicePresent(); }
    bool devicePresent() override;

  private:
    enum REG : uint8_t {
        CONFIG_A = 0x00,
        CONFIG_B = 0x01,
        MODE = 0x02,
        DATA_X_MSB = 0x03,
        STATUS = 0x09,
        ID_A = 0x0a
    };

    static std::int16_t decodeInt16(std::uint8_t highByte, std::uint8_t lowByte);

    [[maybe_unused]] unsigned int fLastConvTime;
    [[maybe_unused]] bool fCalibrationValid;
    unsigned int fGain{1};
    [[maybe_unused]] signed int fCalibParameters[11];
};

#endif // !_HMC5883_H_
