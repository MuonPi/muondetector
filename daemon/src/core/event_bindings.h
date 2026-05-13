#ifndef EVENT_BINDINGS_H
#define EVENT_BINDINGS_H

#include "core/event_bus.h"
#include "core/registries/data_store.h"
#include "data/commands/histogram_request_cmd.h"
#include "data/events/adc_mode_event.h"
#include "data/events/adc_trace_event.h"
#include "data/events/ads1115_event.h"
#include "data/events/bias_switch_event.h"
#include "data/events/bias_voltage_event.h"
#include "data/events/calib_event.h"
#include "data/events/gain_switch_event.h"
#include "data/events/gpio_event.h"
#include "data/events/gpio_inhibit_event.h"
#include "data/events/gpio_rate_event.h"
#include "data/events/i2c_stats_event.h"
#include "data/events/lm75_event.h"
#include "data/events/mcp4728_event.h"
#include "data/events/mqtt_status_event.h"
#include "data/events/pca_switch_event.h"
#include "data/events/polarity_switch_event.h"
#include "data/events/preamp_switch_event.h"
#include "data/events/spi_stats_event.h"
#include "data/events/threshold_setting_event.h"
#include "data/events/ubx_event.h"
#include "data/events/version_event.h"
#include "sinks/tcp_sink.h"
#include "utility/ublox_ratebuffer.h"

class EventBindings {
  public:
    // make tcp sink send data through tcp connections
    inline static void setupTcpSink(EventBus& bus, TcpSink& tcp_sink) {
        bus.subscribe<AdcModeEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<AdcTraceEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<Ads1115Event>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<BiasSwitchEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<BiasVoltageEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<CalibEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<GainSwitchEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<GpioRateEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<GpioInhibitEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<I2CStatsEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<LM75Event>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<MCP4728Event>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<MqttStatusEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<PcaSwitchEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<PolaritySwitchEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<PreampSwitchEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<SPIStatsEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<GpioEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<ThresholdSettingEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<NavSat>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<UbxMsgRates>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<CfgGNSS>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<UbxTimeMarkStruct>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<MonTx>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<MonRx>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<GnssMonHwStruct>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<GnssMonHw2Struct>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<GpsVersion>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<NavStatus>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<UbxTimePulseStruct>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<Histogram>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<LogInfoStruct>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<PositionModeConfig>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<VersionEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
    }

    inline static void setupDatastore(EventBus& bus, DataStore& datastore) {
        // just copied from top
        // bus.subscribe<AdcTraceEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<Ads1115Event>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<BiasSwitchEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<BiasVoltageEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<CalibEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<GainSwitchEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<GpioRateEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<GpioInhibitEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<I2CStatsEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<LM75Event>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<MCP4728Event>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<MqttStatusEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<PcaSwitchEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<PolaritySwitchEvent>([&datastore](const auto& ev) { datastore.store(ev);
        // }); bus.subscribe<PreampSwitchEvent>([&datastore](const auto& ev) { datastore.store(ev);
        // }); bus.subscribe<SPIStatsEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<GpioEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<ThresholdSettingEvent>([&datastore](const auto& ev) { datastore.store(ev);
        // }); bus.subscribe<NavSat>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<UbxMsgRates>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<CfgGNSS>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<UbxTimeMarkStruct>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<MonTx>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<MonRx>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<GnssMonHwStruct>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<GnssMonHw2Struct>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<GpsVersion>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<NavStatus>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<UbxTimePulseStruct>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<Histogram>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<LogInfoStruct>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<PositionModeConfig>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<VersionEvent>([&datastore](const auto& ev) { datastore.store(ev); });

        // set up counter rate buffer for ubx counter
        datastore.ubloxRateBuffer().setBufferTime(std::chrono::seconds(10));
        bus.subscribe<UbxTimeMarkStruct>(
            [&datastore](auto& ev) { datastore.ubloxRateBuffer().onCounterValue(ev.evtCounter); });

        // Setup Histograms
        // bus.subscribe<LM75Event>([&datastore](const auto& ev){
        //     datastore.fill()
        // });
        bus.subscribe<UbxDopStruct>(
            [&datastore](const auto& ev) { datastore.fillHisto("pDOP", 1e-2 * ev.pDOP); });
        //     connect(qtGps, &QtSerialUblox::UBXReceivedDops, this, [this](const UbxDopStruct&
        //     dops) {
        // currentDOP = dops;
        // emit logParameter(LogParameter("positionDOP", QString::number(dops.pDOP / 100.),
        // LogParameter::LOG_AVERAGE)); emit logParameter(LogParameter("timeDOP",
        // QString::number(dops.tDOP / 100.), LogParameter::LOG_AVERAGE)); if
        // (m_histo_map.find("pDOP") != m_histo_map.end()) {
        //     m_histo_map["pDOP"]->fill(1e-2 * dops.pDOP);
        // }
        // if (m_histo_map.find("tDOP") != m_histo_map.end()) {
        //     m_histo_map["tDOP"]->fill(1e-2 * dops.tDOP);
        // }
        // });

        // GeoPosManager
        // bus.subscribe<

        // Send histograms
        bus.subscribe<HistogramRequestCmd>([&bus, &datastore]([[maybe_unused]] const auto&) {
            for (const auto& [name, hist] : datastore.allHistos()) {
                hist->rescale();
                bus.publish(*hist);
            }
        });
    }

    inline static void initAllUbxMsgRate(EventBus& bus) {
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::TIM_TM2, 1, 1});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::TIM_TP, 1, 0});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::NAV_TIMEUTC, 1, 131});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::MON_HW, 1, 47});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::MON_HW2, 1, 49});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::NAV_POSLLH, 1, 43});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::NAV_TIMEGPS, 1, 0});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::NAV_STATUS, 1, 71});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::NAV_CLOCK, 1, 189});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::MON_RXBUF, 1, 53});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::MON_TXBUF, 1, 51});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::NAV_SBAS, 1, 0});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::NAV_DOP, 1, 254});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::MON_RXBUF, 1, 53});
        bus.publish(UbxMsgRateCmd{UBX_MSG::msg_id::MON_RXBUF, 1, 53});
        bus.publish(UbxSetAopCmd{true});
    }

    inline static void pollAllUbxMsgRate(EventBus& bus) {
        bus.publish(UbxMsgPollCmd{UBX_MSG::CFG_PRT});
        bus.publish(UbxMsgPollCmd{UBX_MSG::MON_VER});
        bus.publish(UbxMsgPollCmd{UBX_MSG::CFG_GNSS});
        bus.publish(UbxMsgPollCmd{UBX_MSG::CFG_NAVX5});
        bus.publish(UbxMsgPollCmd{UBX_MSG::CFG_ANT});
        bus.publish(UbxMsgPollCmd{UBX_MSG::CFG_TP5});
        bus.publish(UbxMsgPollCmd{UBX_MSG::CFG_PRT});
        bus.publish(UbxMsgPollCmd{UBX_MSG::CFG_PRT});
    }

  private:
    EventBindings() = delete;
};
#endif // EVENT_BINDINGS_H