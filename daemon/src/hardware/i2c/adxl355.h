#pragma once

#include "device_types.h"
#include "i2c/i2cdevice.h"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

class ADXL355 : public i2cDevice {
public:
    enum class REG : std::uint8_t {
        DEVID_AD   = 0x00,
        DEVID_MST  = 0x01,
        PARTID     = 0x02,

        TEMP2      = 0x06,
        TEMP1      = 0x07,

        XDATA3     = 0x08,
        XDATA2     = 0x09,
        XDATA1     = 0x0A,

        YDATA3     = 0x0B,
        YDATA2     = 0x0C,
        YDATA1     = 0x0D,

        ZDATA3     = 0x0E,
        ZDATA2     = 0x0F,
        ZDATA1     = 0x10,

        FILTER     = 0x28,

        RANGE      = 0x2C,
        POWER_CTL  = 0x2D,
        RESET      = 0x2F
    };

    enum class RANGE {
        _2G,
        _4G,
        _8G
    };

    // Neu
    enum class ODR {
        _4000HZ,
        _2000HZ,
        _1000HZ,
        _500HZ,
        _250HZ,
        _125HZ,
        _62_5HZ,
        _31_25HZ,
        _15_625HZ,
        _7_813HZ,
        _3_906HZ
    };

    // Neu
    enum class HPF {
        OFF,
        _247E_3,
        _62E_3,
        _15E_3,
        _3E_3,
        _0_95E_3,
        _0_24E_3
    };

    // Neu
    enum class MODE {
        STANDBY,
        MEASUREMENT
    };

    struct Acceleration {
        double x;
        double y;
        double z;
    };

    ADXL355();
    ADXL355(const char* busAddress, std::uint8_t slaveAddress);
    ADXL355(std::uint8_t slaveAddress);

    ~ADXL355();

    bool init();
    bool identify();
    bool reset();

    static bool identifyDevice(std::uint8_t address);

    std::optional<double> readX();
    std::optional<double> readY();
    std::optional<double> readZ();
    std::optional<double> readTemperature();

    std::optional<Acceleration> readAcceleration();

    void setRange(RANGE range);
    RANGE getRange() const;

    // Neu
    bool setODR(ODR odr);
    ODR getODR() const;

    bool setHPF(HPF hpf);
    HPF getHPF() const;

    bool setMode(MODE mode);
    MODE getMode() const;

private:
    std::optional<std::int32_t> readAxis(REG reg);

    static const std::unordered_map<REG, std::uint8_t> registerMap;
    static const std::unordered_map<RANGE, std::uint8_t> rangeValueMap;
    static const std::unordered_map<RANGE, double> sensitivityMap;

    // Neu
    static const std::unordered_map<ODR, std::uint8_t> odrValueMap;
    static const std::unordered_map<HPF, std::uint8_t> hpfValueMap;
    static const std::unordered_map<MODE, std::uint8_t> modeValueMap;

    RANGE currentRange = RANGE::_2G;

    // Neu
    ODR currentODR = ODR::_125HZ;
    HPF currentHPF = HPF::OFF;
    MODE currentMode = MODE::STANDBY;
};