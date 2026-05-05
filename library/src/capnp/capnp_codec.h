#ifndef CAPNP_CODEC_H
#define CAPNP_CODEC_H

#include <cstdint>
#include <vector>

struct Ads1115Event;
struct NavSat;
struct GpioEvent;
struct AdcTraceEvent;
struct CalibEvent;
struct CfgGNSS;
struct GpioRateEvent;
struct I2CStatsEvent;
struct MCP4728Event;
struct MonRx;
struct MonTx;
struct PositionModeConfig;
struct UbxMsgRates;
struct UbxTimeMarkStruct;
struct VersionEvent;
struct AdcModeEvent;
struct BiasSwitchEvent;
struct BiasVoltageEvent;
struct GainSwitchEvent;
struct PcaSwitchEvent;
struct PolaritySwitchEvent;
struct PreampSwitchEvent;
struct GpioInhibitEvent;
struct LM75Event;
struct MqttStatusEvent;
struct SPIStatsEvent;
struct ThresholdSettingEvent;
struct GnssPosStruct;
struct GnssMonHwStruct;
struct GnssMonHw2Struct;
struct GpsVersion;
struct NavStatus;
struct UbxTimePulseStruct;
struct LogInfoStruct;
class Histogram;
struct StartBurstSampling;
struct ThresholdSettingCmd;
struct UbxMinCnoCmd;
struct UbxMinMaxSvCmd;
struct UbxMsgPollCmd;
struct UbxMsgPollRateCmd;
struct UbxMsgRateCmd;
struct UbxProtocolSelectionCmd;
struct UbxRateCmd;
struct UbxResetCmd;
struct UbxSaveCmd;
struct UbxSetAopCmd;
struct MqttInhibitCmd;
struct GpioRateResetCmd;
struct UbxConfigDefaultCmd;
struct I2cStatsRequestCmd;
struct I2cScanBusCmd;
struct CalibRequestCmd;
struct CalibSaveCmd;
struct PreampSwitchRequestCmd;
struct GainSwitchRequestCmd;
struct ThresholdRequestCmd;
struct PcaSwitchRequestCmd;
struct AdcModeRequestCmd;
struct PolaritySwitchRequestCmd;
struct BiasVoltageRequestCmd;
struct BiasSwitchRequestCmd;
struct DacRequestCmd;
struct AdcSampleRequestCmd;
struct TemperatureRequestCmd;
struct DacEepromSetCmd;
struct HistogramClearCmd;
struct GpioRateRequestCmd;

template <typename T>
struct CapnpCodec {
    static auto encode(const T&) -> std::vector<std::uint8_t> {
        static_assert(sizeof(T) == 0, "No CapnpCodec specialization for this type");
        return {};
    }

    static auto decode(const std::vector<std::uint8_t>&) -> T {
        static_assert(sizeof(T) == 0, "No CapnpCodec specialization for this type");
        return {};
    }

    static auto messageKey() -> std::uint16_t {
        static_assert(sizeof(T) == 0, "No messageKey specialization for this type");
        return 0;
    }
};

#define DECLARE_CODEC(TYPE)                                                                        \
    template <>                                                                                    \
    struct CapnpCodec<TYPE> {                                                                      \
        static auto encode(const TYPE&) -> std::vector<std::uint8_t>;                              \
        static auto decode(const std::vector<std::uint8_t>& data) -> TYPE;                         \
        static auto messageKey() -> std::uint16_t;                                                 \
    };

DECLARE_CODEC(Ads1115Event)
DECLARE_CODEC(NavSat)
DECLARE_CODEC(GpioEvent)
DECLARE_CODEC(AdcTraceEvent)
DECLARE_CODEC(CalibEvent)
DECLARE_CODEC(CfgGNSS)
DECLARE_CODEC(GpioRateEvent)
DECLARE_CODEC(I2CStatsEvent)
DECLARE_CODEC(MCP4728Event)
DECLARE_CODEC(MonRx)
DECLARE_CODEC(MonTx)
DECLARE_CODEC(PositionModeConfig)
DECLARE_CODEC(UbxMsgRates)
DECLARE_CODEC(UbxTimeMarkStruct)
DECLARE_CODEC(VersionEvent)
DECLARE_CODEC(AdcModeEvent)
DECLARE_CODEC(BiasSwitchEvent)
DECLARE_CODEC(BiasVoltageEvent)
DECLARE_CODEC(GainSwitchEvent)
DECLARE_CODEC(PcaSwitchEvent)
DECLARE_CODEC(PolaritySwitchEvent)
DECLARE_CODEC(PreampSwitchEvent)
DECLARE_CODEC(GpioInhibitEvent)
DECLARE_CODEC(LM75Event)
DECLARE_CODEC(MqttStatusEvent)
DECLARE_CODEC(SPIStatsEvent)
DECLARE_CODEC(ThresholdSettingEvent)
DECLARE_CODEC(GnssPosStruct)
DECLARE_CODEC(GnssMonHwStruct)
DECLARE_CODEC(GnssMonHw2Struct)
DECLARE_CODEC(GpsVersion)
DECLARE_CODEC(NavStatus)
DECLARE_CODEC(UbxTimePulseStruct)
DECLARE_CODEC(LogInfoStruct)
DECLARE_CODEC(Histogram)
DECLARE_CODEC(StartBurstSampling)
DECLARE_CODEC(ThresholdSettingCmd)
DECLARE_CODEC(UbxMinCnoCmd)
DECLARE_CODEC(UbxMinMaxSvCmd)
DECLARE_CODEC(UbxMsgPollCmd)
DECLARE_CODEC(UbxMsgPollRateCmd)
DECLARE_CODEC(UbxMsgRateCmd)
DECLARE_CODEC(UbxProtocolSelectionCmd)
DECLARE_CODEC(UbxRateCmd)
DECLARE_CODEC(UbxResetCmd)
DECLARE_CODEC(UbxSaveCmd)
DECLARE_CODEC(UbxSetAopCmd)
DECLARE_CODEC(MqttInhibitCmd)
DECLARE_CODEC(GpioRateResetCmd)
DECLARE_CODEC(UbxConfigDefaultCmd)
DECLARE_CODEC(I2cStatsRequestCmd)
DECLARE_CODEC(I2cScanBusCmd)
DECLARE_CODEC(CalibRequestCmd)
DECLARE_CODEC(CalibSaveCmd)
DECLARE_CODEC(PreampSwitchRequestCmd)
DECLARE_CODEC(GainSwitchRequestCmd)
DECLARE_CODEC(ThresholdRequestCmd)
DECLARE_CODEC(PcaSwitchRequestCmd)
DECLARE_CODEC(AdcModeRequestCmd)
DECLARE_CODEC(PolaritySwitchRequestCmd)
DECLARE_CODEC(BiasVoltageRequestCmd)
DECLARE_CODEC(BiasSwitchRequestCmd)
DECLARE_CODEC(DacRequestCmd)
DECLARE_CODEC(AdcSampleRequestCmd)
DECLARE_CODEC(TemperatureRequestCmd)
DECLARE_CODEC(DacEepromSetCmd)
DECLARE_CODEC(HistogramClearCmd)
DECLARE_CODEC(GpioRateRequestCmd)
#undef DECLARE_CODEC
#endif // CAPNP_CODEC_H
