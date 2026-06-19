#ifndef _LTR390UV01_H_
#define _LTR390UV01_H_
#include "hardware/device_types.h"
#include "hardware/i2c/i2cdevice.h"

#include <cstdint>
#include <unordered_map>

class LTR390UV01 : public i2cDevice,
                   public DeviceFunction<DeviceType::OTHER>,
                   public static_device_base<LTR390UV01> {
  public:
    enum class REG {
        MAIN_CTRL,
        MEAS_CONFIG,
        GAIN,
        PART_ID,
        MAIN_STATUS,
        ALSDATA,
        UVSDATA,
        INT_CFG,
    };

    enum class GAIN { _1, _3, _6, _9, _18 };

    enum class RESOLUTION {
        _20bit, // Conversion time = 400ms
        _19bit, // Conversion time = 200ms
        _18bit, // Conversion time = 100ms(default)
        _17bit, // Conversion time = 50ms
        _16bit, // Conversion time = 25ms
        _13bit, // Conversion time = 12.5ms
    };

    enum class INTERVAL { _25ms, _50ms, _100ms, _200ms, _500ms, _1000ms, _2000ms };

    enum class MODE { ALS, UVS };

    struct Status {
        bool powerOnStatus{0};
        bool interruptStatus{0};
        bool dataStatus{0};
    };

    LTR390UV01();
    LTR390UV01(const char* busAddress, uint8_t slaveAddress);
    LTR390UV01(uint8_t slaveAddress);
    virtual ~LTR390UV01();
    // float getTemperature() override;

    void init();
    auto id() -> std::uint8_t;
    void setResolution(RESOLUTION resolution);
    void setInterval(INTERVAL interval);
    void setGain(GAIN gain);
    void setInterrupt(MODE mode = MODE::UVS, bool isEnabled = true);
    auto read() -> double;
    auto mainStatus() -> Status;

    bool identify() override;
    bool probeDevicePresence() override { return devicePresent(); }
    bool devicePresent() override;

  protected:
    auto readUVS() -> std::uint32_t;
    auto readALS() -> std::uint32_t;
    MODE currentMode{MODE::UVS}; // Ultraviolet light only
    GAIN currentGain{GAIN::_3};
    RESOLUTION currentResolution{RESOLUTION::_17bit};
    INTERVAL currentInterval{INTERVAL::_100ms};

    std::uint8_t meas_and_rate_register{0x22}; // Default 010 -> 18 Bit, 010 -> 100ms
    static const std::unordered_map<REG, std::uint8_t> registerMap;
    static const std::unordered_map<GAIN, std::uint8_t> gainSetting;
    static const std::unordered_map<RESOLUTION, std::uint8_t> resolutionSetting;
    static const std::unordered_map<INTERVAL, std::uint8_t> intervalSetting;

    static const std::unordered_map<GAIN, double> gainCorrection;
    static const std::unordered_map<RESOLUTION, double> resolutionCorrection;
    // enum REG : uint8_t { TEMP = 0x00, CONF = 0x01, THYST = 0x02, TOS = 0x03 };

    unsigned int fLastConvTime;
    signed int fLastRawValue;
};
#endif // !_LTR390UV01_H_
