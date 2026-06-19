#ifndef _AS7343_H_
#define _AS7343_H_
#include "hardware/device_types.h"
#include "hardware/i2c/i2cdevice.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class AS7343 : public i2cDevice,
               public DeviceFunction<DeviceType::OTHER>,
               public static_device_base<AS7343> {
  public:
    enum class REG {
        AUXID,
        REVID,
        ID,
        CFG12,
        ENABLE,
        ATIME,
        ASTEP,
        WTIME,
        SP_TH_L,
        SP_TH_H,
        STATUS1,
        STATUS2,
        STATUS3,
        STATUS4,
        STATUS5,
        ASTATUS,
        DATA,
        DATA_END,
        CFG0,
        CFG1,
        CFG2,
        CFG3,
        CFG4,
        CFG5,
        CFG6,
        CFG7,
        CFG8,
        CFG9,
        CFG10,
        PERS,
        GPIO,
        CFG20,
        LED,
        AGC_GAIN_MAX,
        AZ_CONFIG,
        FD_TIME_1,
        FD_TIME_2,
        FIFO_CFG0,
        FD_CFG_0,
        FD_STATUS,
        INTENAB,
        CONTROL,
        FIFO_MAP,
        FIFO_LVL,
        FDATA
    };

    enum class GAIN {
        _0dot5x,
        _1x,
        _2x,
        _4x,
        _8x,
        _16x,
        _32x,
        _64x,
        _128x,
        _256x,
        _512x,
        _1024x,
        _2048x
    };

    enum class FIFO_THRESHOLD { _1, _4, _8, _16 };

    enum class SpectralChannel { F1, F2, F3, F4, F5, F6, F7, F8, Z, Y, FXL, VIS, NIR, FD };

    enum class AUTO_SMUX_MODE { _6Ch, _12Ch, _18Ch };

    struct SpectralValue {
        SpectralChannel channel;
        std::uint16_t value;
    };

    // /**
    //  * There are 6 ADC paths which can be routed to some photodiode spectral channel
    //  */
    // struct SmuxConfig
    // {
    //     SpectralChannel ch0;
    //     SpectralChannel ch1;
    //     SpectralChannel ch2;
    //     SpectralChannel ch3;
    //     SpectralChannel ch4;
    //     SpectralChannel ch5;
    // };

    enum class ChannelType { Spectral, Vis, Flicker, Unknown };

    struct SpectralChannelInfo {
        std::string name;
        int minWavelength;
        int centerWavelength;
        int maxWavelength;
        int bandwidth; // or sensitivity / weighting depending on your meaning
        ChannelType type;
    };

    struct FifoFrame {
        std::uint8_t fifoMap{};
        std::vector<std::uint16_t> channelData;
        std::uint8_t status{};
    };

    static const std::unordered_map<SpectralChannel, SpectralChannelInfo> channelInfo;
    static const std::array<SpectralChannel, 18> spectralChannels;
    // static const std::array<SpectralChannelInfo, 12> channelInfo;

    struct ID {
        std::uint8_t auxid;
        std::uint8_t revid;
        std::uint8_t id;
    };

    struct Config {
        GAIN gain{GAIN::_8x};
        bool autoExposure{true};
        bool flickerDetectionEnable{false};
        bool waitEnable{false}; // Wait time between two consecutive spectral measurements
        std::uint8_t gpioMode{0b10};
        std::uint8_t atime{0x00};
        std::uint16_t astep{999}; // defaults to 2.78ms (999 + 1 = 1000 * 2.78µs = step size 2.78ms)
        std::uint8_t wtime{0x00};     // Wait time ( (n+1) * 2.78ms )
        std::uint16_t fdTime{0x0167}; // Note: Must not be changed during FDEN = 1 and PON = 1.
        std::uint16_t fdGain{0x09};
        std::uint8_t autoZeroFreq{0xff}; // Auto zero of spectral engine every n-th integration
                                         // cycle (0xff = only before first measurement cycle)
        std::uint8_t agcFdGainMax{0x09}; // Flicker Detection AGC Gain Max. (2^𝐴𝐺𝐶_𝐹𝐷_𝐺𝐼𝐴𝑁_𝑀𝐴𝑋)
        /**
         * VALUE FIFO_LVL
           0     1
           1     4
           2     8
           3     16
         */
        FIFO_THRESHOLD fifoThreshold{
            FIFO_THRESHOLD::_8}; // Sets a threshold on the FIFO level that triggers the
                                 // first FIFO buffer interrupt (FINT).

        bool ledAct{false};
        /*  LED Driving Strength.
            000 0000: 4 mA
            000 0001: 6 mA
            000 0010: 8 mA
            000 0011: 10 mA
            000 0100: 12 mA
        */
        std::uint8_t ledDrive{0b100};
        bool asien{false}; // Spectral and Flicker Detect Saturation Interrupt Enable.
                           // When asserted permits saturation interrupts to be generated.
        bool spIen{false}; // Spectral Interrupt Enable. Mirrored in the ENABLE register
        bool fIen{false};  // FIFO Buffer Interrupt Enable.
        bool sIen{};       // System Interrupt Enable.
                           // When asserted permits system interrupts to be
                           // generated. Indicates that flicker detection status has
                           // changed or SMUX operation has finished.
        std::uint8_t fifoMap{0x00};
        AUTO_SMUX_MODE autoSmuxMode{AUTO_SMUX_MODE::_6Ch};
        std::uint16_t autoExposureLowTarget{1000};
        std::uint16_t autoExposureHighTarget{50000};
    };

    struct Status {
        // =========================================================
        // STATUS (0x93)
        // =========================================================
        bool asat{false}; // bit 7 - Spectral & Flicker Detect Saturation
        bool aint{false}; // bit 3 - Spectral Channel Interrupt
        bool fint{false}; // bit 2 - FIFO Buffer Interrupt
        bool sint{false}; // bit 0 - System Interrupt

        // =========================================================
        // STATUS2
        // =========================================================
        bool avalid{false};       // bit 6 - Spectral measurement completed
        bool asatDigital{false};  // bit 4 - Digital saturation
        bool asatAnalog{false};   // bit 3 - Analog saturation
        bool fdsatAnalog{false};  // bit 1 - Flicker analog saturation
        bool fdsatDigital{false}; // bit 0 - Flicker digital saturation

        // =========================================================
        // STATUS3
        // =========================================================
        bool intSpH{false}; // bit 5 - Spectral interrupt high
        bool intSpL{false}; // bit 4 - Spectral interrupt low

        // =========================================================
        // STATUS4
        // =========================================================
        bool fifoOv{false};    // bit 7 - FIFO overflow
        bool ovTemp{false};    // bit 5 - Over temperature
        bool fdTrig{false};    // bit 4 - Flicker detect trigger error
        bool spTrig{false};    // bit 2 - Spectral trigger error
        bool saiActive{false}; // bit 1 - Sleep after interrupt active
        bool intBusy{false};   // bit 0 - Initialization busy

        // =========================================================
        // STATUS5
        // =========================================================
        bool sintFd{false};   // bit 3 - Flicker detect interrupt
        bool sintSmux{false}; // bit 2 - SMUX operation complete
    };

    struct FdStatus {
        bool fdMeasurementValid{false};
        bool fdSaturationDetected{false};
        bool fd120HzFlickerValid{false};
        bool fd120HzFlicker{false}; // Flicker Detected at 120 Hz.
        bool fd100HzFlicker{false}; // Flicker Detected at 100 Hz.
    };

    struct AStatus {
        bool asatStatus{false};  // Saturation Status. Indicates if the latched data is affected by
                                 // analog or digital saturation.
        bool aGainStatus{false}; // Gain Status. Indicates the gain applied for the spectral data
                                 // latched to this ASTATUS read.
    };

    AS7343();
    AS7343(const char* busAddress, uint8_t slaveAddress);
    AS7343(uint8_t slaveAddress);
    virtual ~AS7343();

    void reset();
    void powerOn();
    void init(const Config& conf);
    auto name() const -> std::string;
    auto ids() -> ID;
    auto status() -> Status;
    auto fdStatus() -> FdStatus;
    auto astatus() -> AStatus;
    auto readSpectrum() -> std::vector<SpectralValue>;
    auto getConfig() const -> Config;

    void setGain(GAIN gain);
    void setIntegrationTime(std::uint8_t atime, std::uint16_t astep);
    void setWait(bool enable, std::uint8_t wtime, bool longWait = false);
    void setLed(bool active, std::uint8_t drive);
    void setGpioMode(std::uint8_t mode);
    void setInterrupts(bool saturation, bool spectral, bool fifo, bool system);
    void setSpectralInterruptThresholds(std::uint16_t low, std::uint16_t high, std::uint8_t channel,
                                        std::uint8_t persistence);
    void setFlickerDetection(bool enable, std::uint16_t fdTime, GAIN fdGain,
                             std::uint8_t agcGainMax);
    void setAutoZeroFrequency(std::uint8_t frequency);
    void setAutoExposure(bool enable);
    void manualAutoZero();

    static auto toString(const Status& status) -> std::string;

    void printSpectrum(const std::vector<SpectralValue>& values);
    // auto toPhysical(const std::vector<SpectralChannel>& spectrum) -> std::unordered_map<float,
    // float>; // wavelength/value

    bool identify() override;
    bool probeDevicePresence() override { return devicePresent(); }
    bool devicePresent() override;

  protected:
    static const std::unordered_map<AUTO_SMUX_MODE, std::vector<SpectralChannel>>
        autoSmuxChannelMapping;

    std::vector<SpectralValue> spectrum;
    std::chrono::time_point<std::chrono::steady_clock> lastMeasurementCompleted;
    Config config;
    ID deviceId{};
    std::uint16_t adcFullscale =
        0xffff; // 𝐴𝐷𝐶 𝑓𝑢𝑙𝑙𝑠𝑐𝑎𝑙𝑒 = (𝐴𝑇𝐼𝑀𝐸 + 1) × (𝐴𝑆𝑇𝐸𝑃 + 1) but maximum 65535 (2^16 - 1) == 0xffff
    static const std::unordered_map<REG, std::uint8_t> registerMap;
    static const std::unordered_map<GAIN, std::uint8_t> gainConfigMap;

    auto readIds() -> bool;
    void startMeasurement();
    void stopMeasurement();
    void setAutoSMUX(AUTO_SMUX_MODE mode);
    void setFifoMap(std::uint8_t fifoMap);
    void setFifoThreshold(FIFO_THRESHOLD threshold);
    void waitForSMUX();

    void waitForSpectrum();

    auto integrationTime() -> float;
    auto adcFullScale() const -> std::uint16_t;
    auto gainValue(GAIN gain) const -> std::uint8_t;
    auto adjustExposureIfNeeded(const std::vector<SpectralValue>& values,
                                const Status& status) -> bool;

    // FIFO operations
    auto fifoFrameSize() -> std::size_t;
    auto fifoLevel() -> std::size_t;
    void clearFifo();
    auto readFifoFrame() -> std::optional<FifoFrame>;

    // Register access
    void setRegisterLowRange(bool low);
};
#endif // !_AS7343_H_
