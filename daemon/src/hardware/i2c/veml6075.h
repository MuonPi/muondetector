#ifndef _VEML6075_H_
#define _VEML6075_H_

#include "device_types.h"
#include "i2c/i2cdevice.h"

#include <array>
#include <cstdint>

class VEML6075 : public i2cDevice,
                 public DeviceFunction<DeviceType::LIGHT>,
                 public static_device_base<VEML6075> {
  public:
    static constexpr std::uint8_t DEFAULT_ADDRESS{0x10};
    static constexpr std::uint16_t EXPECTED_DEVICE_ID{0x0026};

    struct RawReading {
        std::uint16_t uva{0};
        std::uint16_t uvb{0};
        std::uint16_t uvComp1{0};
        std::uint16_t uvComp2{0};
        bool valid{false};
    };

    struct UVReading {
        double uvaIndex{0.0};
        double uvbIndex{0.0};
        double uvIndex{0.0};
        RawReading raw{};
        bool valid{false};
    };

    VEML6075();
    VEML6075(const char* busAddress, std::uint8_t slaveAddress);
    VEML6075(std::uint8_t slaveAddress);
    virtual ~VEML6075();

    bool init();
    bool setConfig();
    bool readRaw(RawReading& reading);
    UVReading readUV();
    bool readUV(UVReading& reading);

    bool getUVRawValue(std::int16_t* value);
    bool getUV(double* uv);

    bool identify() override;
    bool probeDevicePresence() override { return devicePresent(); }
    bool devicePresent() override;

    std::uint16_t deviceId();

    std::uint8_t UV_IT{0b001};
    std::uint8_t HD{0b1};
    std::uint8_t UV_TRIG{0b0};
    std::uint8_t UV_AF{0b0};
    std::uint8_t AD{0b0};

  private:
    enum REG : std::uint8_t {
        CONF = 0x00,
        UVA_DATA = 0x07,
        UVB_DATA = 0x09,
        UVCOMP1_DATA = 0x0a,
        UVCOMP2_DATA = 0x0b,
        ID = 0x0c
    };

    bool readWordLittleEndian(REG reg, std::uint16_t& value);
    std::uint8_t configByte() const;

    static constexpr double UVA_VIS_COEFF{2.22};
    static constexpr double UVA_IR_COEFF{1.33};
    static constexpr double UVB_VIS_COEFF{2.95};
    static constexpr double UVB_IR_COEFF{1.74};
    static constexpr double UVA_RESPONSIVITY{0.001461};
    static constexpr double UVB_RESPONSIVITY{0.002591};
};

#endif // _VEML6075_H_
