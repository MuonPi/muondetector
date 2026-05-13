#include "sources/tcp_command_decoder.h"

#include "capnp/capnp_codec.h"
#include "core/logging/logger.h"
#include "data/commands/adc_mode_request_cmd.h"
#include "data/commands/adc_sample_request_cmd.h"
#include "data/commands/bias_switch_cmd.h"
#include "data/commands/bias_voltage_cmd.h"
#include "data/commands/burst_sampling_cmd.h"
#include "data/commands/calibration_cmd.h"
#include "data/commands/calibration_save_cmd.h"
#include "data/commands/dac_cmd.h"
#include "data/commands/dac_eeprom_set_cmd.h"
#include "data/commands/gain_switch_cmd.h"
#include "data/commands/gpio_rate_request_cmd.h"
#include "data/commands/gpio_rate_reset_cmd.h"
#include "data/commands/histogram_clear_cmd.h"
#include "data/commands/i2c_scan_bus_cmd.h"
#include "data/commands/i2c_stats_request_cmd.h"
#include "data/commands/mqtt_inhibit_cmd.h"
#include "data/commands/pca_switch_cmd.h"
#include "data/commands/polarity_switch_cmd.h"
#include "data/commands/position_mode_cmd.h"
#include "data/commands/preamp_switch_cmd.h"
#include "data/commands/temperature_request_cmd.h"
#include "data/commands/threshold_setting_cmd.h"
#include "data/commands/ubx_config_default_cmd.h"
#include "data/commands/ubx_gnss_config_cmd.h"
#include "data/commands/ubx_msg_poll_cmd.h"
#include "data/commands/ubx_msg_poll_rate_cmd.h"
#include "data/commands/ubx_msg_rate_cmd.h"
#include "data/commands/ubx_reset_cmd.h"
#include "data/commands/ubx_save_cmd.h"
#include "data/commands/ubx_tp5_cmd.h"
#include "network/tcpmessage_keys.h"

#include <exception>
#include <string>

TcpCommandDecoder::TcpCommandDecoder(ComponentId id, EventBus& bus) : Component(id), bus_(bus) {
    bus_.subscribe<TcpPacketEvent>([this](const TcpPacketEvent& event) { this->handle(event); });
    if (!std::holds_alternative<OtherComponent>(id)) {
        throw std::logic_error("NonDeviceSource constructed with device ID");
    }
}

void TcpCommandDecoder::handle(const TcpPacketEvent& event) {
    const auto key = static_cast<TCP_MSG_KEY>(event.packet.key);

    logDebug("Received command " + std::to_string(event.packet.key));

    try {
        switch (key) {
            case TCP_MSG_KEY::MSG_UBX_CFG_TP5: {
                auto tp5 = CapnpCodec<UbxTimePulseStruct>::decode(event.packet.payload);
                bus_.publish(static_cast<UbxTp5Cmd>(std::move(tp5)));
                break;
            }
            case TCP_MSG_KEY::MSG_UBX_GNSS_CONFIG: {
                bus_.publish(CapnpCodec<UbxGnssConfigCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_UBX_MSG_RATE: {
                bus_.publish(CapnpCodec<UbxMsgRateCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_UBX_MSG_POLL: {
                bus_.publish(CapnpCodec<UbxMsgPollCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_UBX_MSG_RATE_REQUEST: {
                bus_.publish(CapnpCodec<UbxMsgPollRateCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_UBX_RESET: {
                bus_.publish(CapnpCodec<UbxResetCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_UBX_CFG_SAVE: {
                bus_.publish(CapnpCodec<UbxSaveCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_UBX_CONFIG_DEFAULT: {
                bus_.publish(CapnpCodec<UbxConfigDefaultCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_RATE_SCAN: {
                bus_.publish(CapnpCodec<StartBurstSampling>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_MQTT_INHIBIT: {
                bus_.publish(CapnpCodec<MqttInhibitCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_GPIO_RATE_RESET: {
                bus_.publish(CapnpCodec<GpioRateResetCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_I2C_STATS_REQUEST: {
                bus_.publish(CapnpCodec<I2cStatsRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_I2C_SCAN_BUS: {
                bus_.publish(CapnpCodec<I2cScanBusCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_CALIB_REQUEST: {
                bus_.publish(CapnpCodec<CalibRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_CALIB_SAVE: {
                bus_.publish(CapnpCodec<CalibSaveCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_PREAMP_SWITCH: {
                bus_.publish(CapnpCodec<PreampSwitchCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_PREAMP_SWITCH_REQUEST: {
                bus_.publish(CapnpCodec<PreampSwitchRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_GAIN_SWITCH: {
                bus_.publish(CapnpCodec<GainSwitchCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_GAIN_SWITCH_REQUEST: {
                bus_.publish(CapnpCodec<GainSwitchRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_THRESHOLD: {
                bus_.publish(CapnpCodec<ThresholdSettingCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_THRESHOLD_REQUEST: {
                bus_.publish(CapnpCodec<ThresholdSettingRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_PCA_SWITCH: {
                bus_.publish(CapnpCodec<PcaSwitchCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_ADC_MODE_REQUEST: {
                bus_.publish(CapnpCodec<AdcModeRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_POLARITY_SWITCH: {
                bus_.publish(CapnpCodec<PolaritySwitchCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_POLARITY_SWITCH_REQUEST: {
                bus_.publish(CapnpCodec<PolaritySwitchRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_BIAS_VOLTAGE: {
                bus_.publish(CapnpCodec<BiasVoltageCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_BIAS_SWITCH_REQUEST: {
                bus_.publish(CapnpCodec<BiasSwitchRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_DAC_SET: {
                bus_.publish(CapnpCodec<DacCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_DAC_REQUEST: {
                bus_.publish(CapnpCodec<DacRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_DAC_EEPROM_SET: {
                bus_.publish(CapnpCodec<DacEepromSetCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_ADC_SAMPLE_REQUEST: {
                bus_.publish(CapnpCodec<AdcSampleRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_TEMPERATURE_REQUEST: {
                bus_.publish(CapnpCodec<TemperatureRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_HISTOGRAM_CLEAR: {
                bus_.publish(CapnpCodec<HistogramClearCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_GPIO_RATE_REQUEST: {
                bus_.publish(CapnpCodec<GpioRateRequestCmd>::decode(event.packet.payload));
                break;
            }
            case TCP_MSG_KEY::MSG_POSITION_MODEL: {
                auto cfg = CapnpCodec<PositionModeConfig>::decode(event.packet.payload);
                bus_.publish(static_cast<PositionModeCmd>(std::move(cfg)));
                break;
            }
            default:
                break;
        }
    } catch (const std::exception& ex) {
        logWarn("TcpCommandDecoder failed to decode key " + std::to_string(event.packet.key) +
                ": " + ex.what());
    }
}
