#ifndef _AS7331_H_
#define _AS7331_H_
#include "device_types.h"
#include "i2c/i2cdevice.h"

#include <cstdint>
#include <unordered_map>

class AS7331 : public i2cDevice,
               public DeviceFunction<DeviceType::OTHER>,
               public static_device_base<AS7331> {
  public:
    enum class REG {
        OSR_STATUS,
        TEMP,
        AGEN,
        MRES_A,
        MRES_B,
        MRES_C,
        OUTCONV_L,
        OUTCONV_H,
        CREG_1,
        CREG_2,
        CREG_3,
        BREAK,
        EDGES,
        OPTREG
    };

    enum class GAIN { _2048x, _1024x, _512x, _256x, _128x, _64x, _32x, _16x, _8x, _4x, _2x, _1x };

    /**
     * Measurement Mode
     */
    enum class MMODE {
        CONT, // continuous measurement
        CMD,  // measurement per command
        SYNS, // externally synchronized start of measurement
        SYND  // start and end of measurement are externally synchronized
    };

    /**
     * Internal clock frequency
     */
    enum class CCLK { _1024MHz, _2048MHz, _4096MHz, _8192MHz };

    enum class RDYOD { PIN_READY_PUSH_PULL, PIN_READY_OPEN_DRAIN };

    enum class OP_STATE { CONFIGURATION, MEASUREMENT, UNKNOWN };

    struct Status {
        bool startOfMeasurement{false};
        bool powerDownState{true};
        OP_STATE opState{OP_STATE::CONFIGURATION};
        bool outConvOf{false};
        bool mresOf{false};
        bool adcOf{false};
        bool lData{false};
        bool nData{false};
        bool notReady{false};
        bool standby{false};
        bool powerDown{false};
    };

    AS7331();
    AS7331(const char* busAddress, uint8_t slaveAddress);
    AS7331(uint8_t slaveAddress);
    virtual ~AS7331();

    void reset();
    auto opStatus() -> Status;
    void setOpState(OP_STATE state);
    void setGain(GAIN gain);
    void startMeasurement();
    void setBreakTime(std::uint8_t time); // Step size 8µs
    // void setStandby(bool standby);

    auto getConfig() -> std::string;

    auto getAGEN() -> std::uint8_t;
    // void setIntegrationTime(std::uint8_t value); // TCONV = 2^value ms (EXCEPTION: value == 15 ->
    // TCONV = 1 ms)

    auto readUVA() -> std::optional<double>; // [µW/cm^2]
    auto readUVB() -> std::optional<double>; // [µW/cm^2]
    auto readUVC() -> std::optional<double>; // [µW/cm^2]

    static auto toString(const Status& status) -> std::string;

    bool identify() override;
    bool probeDevicePresence() override { return devicePresent(); }
    bool devicePresent() override;

  protected:
    static const std::unordered_map<REG, std::uint8_t> registerMap;
    static const std::unordered_map<GAIN, std::uint8_t> gainValueMap;
    /**
     * Full Scale Range E_e [µW/cm^2] -> Effective LSB is fullScaleRange / (2^16 - 1)
     */
    static const std::unordered_map<GAIN, double> fullScaleRangeMapUVA;
    static const std::unordered_map<GAIN, double> fullScaleRangeMapUVB;
    static const std::unordered_map<GAIN, double> fullScaleRangeMapUVC;

    OP_STATE currentOpState{OP_STATE::CONFIGURATION};
    GAIN currentGain{GAIN::_2x}; // default
    std::uint8_t currentTime{0b0110};
    std::uint8_t currentBreakTime{};
    MMODE currentMMode{MMODE::CMD};
    CCLK currentCCLK{CCLK::_1024MHz};
    RDYOD currentRDYOD{RDYOD::PIN_READY_PUSH_PULL};
    bool standby{false};
    bool EN_TM = true;
    bool EN_DIV = false; // Digital divider of the measurement result registers might be needed
                         // @CREG1:TIME > 6 dec
    auto readMRES(REG reg) -> std::optional<std::uint16_t>;
};
#endif // !_AS7331_H_
