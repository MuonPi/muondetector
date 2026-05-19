#ifndef EVENT_BINDINGS_H
#define EVENT_BINDINGS_H

#include "core/event_bus.h"
#include "core/registries/data_store.h"
#include "data/commands/calibration_cmd.h"
#include "data/commands/dac_setting_request_cmd.h"
#include "data/commands/event_trigger_cmd.h"
#include "data/commands/gain_switch_cmd.h"
#include "data/commands/gpio_rate_request_cmd.h"
#include "data/commands/histogram_request_cmd.h"
#include "data/commands/i2c_stats_request_cmd.h"
#include "data/commands/polarity_switch_cmd.h"
#include "data/commands/preamp_switch_cmd.h"
#include "data/commands/spi_stats_request_cmd.h"
#include "data/commands/temperature_request_cmd.h"
#include "data/commands/threshold_setting_cmd.h"
#include "data/events/adc_mode_event.h"
#include "data/events/adc_trace_event.h"
#include "data/events/ads1115_event.h"
#include "data/events/bias_switch_event.h"
#include "data/events/bias_voltage_event.h"
#include "data/events/calib_event.h"
#include "data/events/datastore_store_event.h"
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

#include <array>
#include <type_traits>

const std::vector<UBX_MSG::msg_id> rateCfgMsgID{
    {UBX_MSG::TIM_TM2,       UBX_MSG::TIM_TP,      UBX_MSG::NAV_CLOCK,   UBX_MSG::NAV_DGPS,
     UBX_MSG::NAV_AOPSTATUS, UBX_MSG::NAV_DOP,     UBX_MSG::NAV_POSECEF, UBX_MSG::NAV_POSLLH,
     UBX_MSG::NAV_PVT,       UBX_MSG::NAV_SBAS,    UBX_MSG::NAV_SOL,     UBX_MSG::NAV_STATUS,
     UBX_MSG::NAV_SVINFO,    UBX_MSG::NAV_TIMEGPS, UBX_MSG::NAV_TIMEUTC, UBX_MSG::NAV_VELECEF,
     UBX_MSG::NAV_VELNED,    UBX_MSG::MON_HW,      UBX_MSG::MON_HW2,     UBX_MSG::MON_IO,
     UBX_MSG::MON_MSGPP,     UBX_MSG::MON_RXBUF,   UBX_MSG::MON_RXR,     UBX_MSG::MON_TXBUF}};

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
        bus.subscribe<NavClock>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<UbxTimePulseStruct>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<Histogram>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<LogInfoStruct>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<PositionModeConfig>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
        bus.subscribe<VersionEvent>([&tcp_sink](const auto& ev) { tcp_sink.handle(ev); });
    }

    // All Events are either published wrapped inside of DataStoreStoreEvent or "normally"
    // If published wrapped, can also contain arrays of Events (for example for threshold events)
    // The array is stored in storage as std::array<event, x> and fan out here by iterating
    // So both stored and not stored events are always sent out to tcp connection
    // IMPORTANT: Never publish both DatastoreStoreEvent<some_event> and some_event because it will
    // be published twice to GUI.
    template <typename T, typename = void>
    struct is_iterable : std::false_type {};

    template <typename T>
    struct is_iterable<T, std::void_t<decltype(std::begin(std::declval<T>())),
                                      decltype(std::end(std::declval<T>()))>> : std::true_type {};

    template <typename T>
    inline static void subscribe_one(EventBus& bus, DataStore& datastore) {
        bus.subscribe<DatastoreStoreEvent<T>>([&datastore, &bus](const DatastoreStoreEvent<T>& ev) {
            datastore.store(ev.data);
            if constexpr (is_iterable<T>::value && !std::is_same_v<T, std::string>) {
                for (const auto& item : ev.data) {
                    bus.publish(item);
                }
            } else {
                bus.publish(ev.data);
            }
        });
    }

    template <typename... Ts>
    inline static void subscribe_all(EventBus& bus, DataStore& datastore) {
        (subscribe_one<Ts>(bus, datastore), ...);
    }

    inline static void setupDatastore(EventBus& bus, DataStore& datastore) {
        // GeoPosManager
        bus.subscribe<NavClock>([&datastore](const NavClock& event) {
            GeoPosManager& geoPosManager = datastore.geoPosManager();
            geoPosManager.set_time_accuracy(event.tAcc);
        });
        bus.subscribe<NavStatus>([&datastore](const NavStatus& event) {
            GeoPosManager& geoPosManager = datastore.geoPosManager();
            geoPosManager.set_fix_status(event.gpsFix);
        });
        // Special Case GnssPosStruct -> do not automatically store this but use custom storage /
        // transformation
        bus.subscribe<DatastoreStoreEvent<GnssPosStruct>>([&bus, &datastore](const auto& event) {
            const auto& pos = event.data;
            datastore.fillHisto(pos);
            GnssPosStruct new_pos_struct{pos};
            GeoPosManager& geoPosManager = datastore.geoPosManager();

            // set correct position errors in case no fix is available (ublox bug?)
            if (geoPosManager.fix_status()().value < Gnss::FixType::Fix2d &&
                geoPosManager.time_precision().age() < std::chrono::seconds(60)) {
                constexpr double c_vacuum{0.3};
                new_pos_struct.hAcc = new_pos_struct.vAcc =
                    c_vacuum * geoPosManager.time_precision()().count() * 1e3;
            }

            geoPosManager.new_position({new_pos_struct.lon * 1e-7, new_pos_struct.lat * 1e-7,
                                        1e-3 * new_pos_struct.hMSL, 1e-3 * new_pos_struct.hAcc,
                                        1e-3 * new_pos_struct.vAcc});
            datastore.store(new_pos_struct);
            bus.publish(new_pos_struct);
        });

        // Currently can only store one event at a time therefore those make no sense
        // bus.subscribe<GpioEvent>([&datastore](const auto& ev) { datastore.store(ev); });
        // bus.subscribe<ThresholdSettingEvent>([&datastore](const auto& ev) { datastore.store(ev);
        // });

        // All events which should be stored are defined here
        // All events with DataStoreStoreEvent<BiasVoltageEvent> ...
        // will be sent to datastore and also sent to GUI via TCP as long as there is a Capnp Codec
        // for it.
        subscribe_all<CfgGNSS, NavSat, MonTx, MonRx, NavStatus, NavClock, UbxTimeMarkStruct,
                      UbxTimePulseStruct, GnssMonHwStruct, GnssMonHw2Struct, NavTimeGPS, NavTimeUTC,
                      BiasVoltageEvent, MqttStatusEvent,
                      AdcTraceEvent, // std::array<Ads1115Event, 4>, // This is deprecated
                      BiasSwitchEvent, CalibEvent, GainSwitchEvent, GpioRateEvent, GpioInhibitEvent,
                      LM75Event, MCP4728Event, PcaSwitchEvent, PolaritySwitchEvent, GpsVersion,
                      EventTriggerEvent, LogInfoStruct, PositionModeConfig, VersionEvent>(
            bus, datastore);

        // Message Requests will be answered directly from datastore
        bus.subscribe<ThresholdSettingRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<MCP4728Event>().has_value()) {
                const auto& value = *datastore.get<MCP4728Event>();
                bus.publish(ThresholdSettingEvent{
                    .channel = 0,
                    .voltage =
                        value.voltages.at(MuonPi::Config::Hardware::DAC::Channel::threshold[0])});
                bus.publish(ThresholdSettingEvent{
                    .channel = 1,
                    .voltage =
                        value.voltages.at(MuonPi::Config::Hardware::DAC::Channel::threshold[1])});
            } else {
                logWarn("Received ThresholdSettingRequestCmd but datastore does not have data for "
                        "type ThresholdSettingEvent.");
            }
        });

        bus.subscribe<BiasVoltageRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<MCP4728Event>().has_value()) {
                const auto& value = *datastore.get<MCP4728Event>();
                bus.publish(BiasVoltageEvent{
                    .voltage = value.voltages.at(MuonPi::Config::Hardware::DAC::Channel::bias)});
            } else {
                logWarn("Received BiasVoltageRequestCmd but datastore does not have data for type "
                        "MCP4728Event.");
            }
        });

        bus.subscribe<PcaSwitchRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<PcaSwitchEvent>().has_value()) {
                bus.publish(*datastore.get<PcaSwitchEvent>());
            } else {
                logWarn("Received PcaSwitchRequestCmd but datastore does not have data for type "
                        "PcaSwitchEvent.");
            }
        });

        bus.subscribe<BiasSwitchRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<BiasSwitchEvent>().has_value()) {
                bus.publish(*datastore.get<BiasSwitchEvent>());
            } else {
                logWarn("Received BiasSwitchRequestCmd but datastore does not have data for type "
                        "BiasSwitchEvent.");
            }
        });

        bus.subscribe<PolaritySwitchRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<PolaritySwitchEvent>().has_value()) {
                bus.publish(*datastore.get<PolaritySwitchEvent>());
            } else {
                logWarn("Received PolaritySwitchRequestCmd but datastore does not have data for "
                        "type PolaritySwitchEvent.");
            }
        });

        bus.subscribe<GainSwitchRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<GainSwitchEvent>().has_value()) {
                bus.publish(*datastore.get<GainSwitchEvent>());
            } else {
                logWarn("Received GainSwitchRequestCmd but datastore does not have data for type "
                        "GainSwitchEvent.");
            }
        });

        bus.subscribe<GpioRateRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<GpioRateEvent>().has_value()) {
                bus.publish(*datastore.get<GpioRateEvent>());
            } else {
                logWarn("Received GpioRateRequestCmd but datastore does not have data for type "
                        "GpioRateEvent");
            }
        });

        bus.subscribe<TemperatureRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<LM75Event>().has_value()) {
                bus.publish(*datastore.get<LM75Event>());
            } else {
                logWarn("Received GpioRateRequestCmd but datastore does not have data for type "
                        "GpioRateEvent");
            }
        });

        // Deprecated: No storing of recent ads1115 events anymore!
        // bus.subscribe<AdcSampleRequestCmd>([&datastore, &bus](const auto& cmd) {
        //     if (datastore.lastUpdate<std::array<Ads1115Event, 4>>().has_value()) {
        //         const auto& data = *datastore.get<std::array<Ads1115Event, 4>>();
        //         bus.publish(data.at(cmd.channel));
        //     } else {
        //         logWarn("Received AdcSampleRequestCmd but datastore does not have data for type "
        //                 "std::array<Ads1115Event, 4>.");
        //     }
        // });

        bus.subscribe<CalibRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<CalibEvent>().has_value()) {
                bus.publish(*datastore.get<CalibEvent>());
            } else {
                logWarn("Received CalibRequestCmd but datastore does not have data for type "
                        "CalibEvent");
            }
        });

        bus.subscribe<SPIStatsRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<SPIStatsEvent>().has_value()) {
                bus.publish(*datastore.get<SPIStatsEvent>());
            } else {
                logWarn("Received SPIStatsRequestCmd but datastore does not have data for type "
                        "SPIStatsEvent");
            }
        });

        bus.subscribe<EventTriggerRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<EventTriggerEvent>().has_value()) {
                bus.publish(*datastore.get<EventTriggerEvent>());
            } else {
                logWarn("Received EventTriggerRequestCmd but datastore does not have data for type "
                        "EventTriggerEvent");
            }
        });

        bus.subscribe<DacSettingRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<MCP4728Event>().has_value()) {
                bus.publish(*datastore.get<MCP4728Event>());
            } else {
                logWarn("Received DacSettingRequestCmd but datastore does not have data for type "
                        "MCP4728Event");
            }
        });

        bus.subscribe<PreampSwitchRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<std::array<PreampSwitchEvent, 2>>().has_value()) {
                auto data = *datastore.get<std::array<PreampSwitchEvent, 2>>();
                bus.publish(data[0]);
                bus.publish(data[1]);
            } else {
                logWarn("Received PreampSwitchRequestCmd but datastore does not have data for type "
                        "std::array<PreampSwitchEvent, 2>.");
            }
        });

        // For PreampSwitchEvent: grab Store event and update std::array<PreampSwitchEvent, 2> and
        // convert
        bus.subscribe<DatastoreStoreEvent<PreampSwitchEvent>>(
            [&datastore, &bus](const auto& event) {
                if (event.data.channel > 1) {
                    logError("Invalid Preamp Switch channel " + std::to_string(event.data.channel));
                    return;
                }
                if (datastore.lastUpdate<std::array<PreampSwitchEvent, 2>>().has_value()) {
                    auto data = *datastore.get<std::array<PreampSwitchEvent, 2>>();
                    data[event.data.channel] = event.data;
                    datastore.store(data);
                    bus.publish(PreampSwitchRequestCmd{});
                } else {
                    std::array<PreampSwitchEvent, 2> data{};
                    data[event.data.channel] = event.data;
                    datastore.store(data);
                }
            });

        bus.subscribe<UbxMsgRateRequestCmd>([&datastore, &bus]([[maybe_unused]] const auto&) {
            if (datastore.lastUpdate<std::unordered_map<std::uint16_t, CfgMsg>>().has_value()) {
                const auto& data = *datastore.get<std::unordered_map<std::uint16_t, CfgMsg>>();
                UbxMsgRates out{};
                out.data.reserve(data.size());
                for (const auto& [key, entry] : data) {
                    out.data.push_back(entry);
                }
                bus.publish(out);
            } else {
                logWarn("Received UbxMsgRateRequestCmd but datastore does not have data for type "
                        "UbxMsgRates");
            }
        });

        // Collect CfgMsg events and update datastore, then on UbxMsgRateRequestCmd build it
        bus.subscribe<CfgMsg>([&datastore](const CfgMsg& msg) {
            logWarn("Storing CfgMsg for ubx message " + std::to_string(msg.msgID));
            // If no data already in the datastore, create new unordered map and store it
            if (datastore.lastUpdate<std::unordered_map<std::uint16_t, CfgMsg>>().has_value()) {
                auto data = *datastore.get<std::unordered_map<std::uint16_t, CfgMsg>>();
                data.emplace(msg.msgID, msg);
                datastore.store(std::move(data));
            } else {
                datastore.store(std::unordered_map<std::uint16_t, CfgMsg>{{msg.msgID, msg}});
            }
        });

        // Convert MCP4728Event to BiasVoltageEvent etc.
        bus.subscribe<MCP4728Event>([&bus](const MCP4728Event& event) {
            bus.publish(ThresholdSettingEvent{
                .channel = 0,
                .voltage =
                    event.voltages.at(MuonPi::Config::Hardware::DAC::Channel::threshold[0])});
            bus.publish(ThresholdSettingEvent{
                .channel = 1,
                .voltage =
                    event.voltages.at(MuonPi::Config::Hardware::DAC::Channel::threshold[1])});
            bus.publish(BiasVoltageEvent{
                .voltage = event.voltages.at(MuonPi::Config::Hardware::DAC::Channel::bias)});
        });

        // set up counter rate buffer for ubx counter
        datastore.ubloxRateBuffer().setBufferTime(std::chrono::seconds(10));
        bus.subscribe<UbxTimeMarkStruct>(
            [&datastore](auto& ev) { datastore.ubloxRateBuffer().onCounterValue(ev.evtCounter); });

        // m_geopos_manager.set_lockin_ready_callback(std::bind(&Daemon::onGeoPosLockInReady, this,
        // std::placeholders::_1));
        // m_geopos_manager.set_valid_pos_callback(std::bind(&Daemon::onGeoPosValid, this,
        // std::placeholders::_1)); m_geopos_manager.set_mode_config(config.position_mode_config);

        // void Daemon::onGeoPosLockInReady(GeoPosition pos)
        // {
        //     qDebug() << "GeoPosition lock-in finished";
        //     sendGeodeticPos(pos.getPosStruct());
        //     sendPositionModel(m_geopos_manager.get_mode_config());

        //     qInfo() << "concluded geo pos lock-in and fixed position: lat=" << pos.latitude
        //             << "lon=" << pos.longitude
        //             << "(err=" << pos.hor_error << "m)"
        //             << "alt=" << pos.altitude
        //             << "(err=" << pos.vert_error << "m)";

        //     writeSettingsToFile();
        // }

        // Histograms
        datastore.setupHistos();
        bus.subscribe<HistogramClearCmd>([&datastore, &bus](const HistogramClearCmd& cmd) {
            bus.publish(datastore.clearHisto(cmd.histogramName));
        });
        bus.subscribe<HistogramRequestCmd>([&bus, &datastore]([[maybe_unused]] const auto&) {
            for (const auto& [name, hist] : datastore.allHistos()) {
                hist->rescale();
                bus.publish(*hist);
            }
        });
    }

    inline static void pollDatastore(EventBus& bus, DataStore& datastore) {
        // This will be called on new connected tcp client like a GUI
        // Sends out all data which is needed on a new connection
        // Gets data preferrably from cached datastore
        bus.publish(BiasSwitchRequestCmd{});
        bus.publish(DacSettingRequestCmd{});
        bus.publish(PcaSwitchRequestCmd{});
        bus.publish(EventTriggerRequestCmd{});
        // bus.publish(MqttStatusRequestCmd{});
        bus.publish(CalibRequestCmd{});
        bus.publish(GainSwitchRequestCmd{});
        bus.publish(GpioRateRequestCmd{});
        bus.publish(TemperatureRequestCmd{});
        bus.publish(I2CStatsRequestCmd{});
        bus.publish(SPIStatsRequestCmd{});
        bus.publish(PolaritySwitchRequestCmd{});
        bus.publish(PreampSwitchRequestCmd{});

        bus.publish(UbxMsgRateRequestCmd{});

        // poll all data from Ubx where there is no RequestCmd:
        if (datastore.lastUpdate<GpsVersion>().has_value()) {
            bus.publish(*datastore.get<GpsVersion>());
        } else {
            logWarn("Datastore does not have data for type "
                    "GpsVersion in pollDatastore function");
        }
        if (datastore.lastUpdate<CfgGNSS>().has_value()) {
            bus.publish(*datastore.get<CfgGNSS>());
        } else {
            logWarn("Datastore does not have data for type "
                    "CfgGNSS in pollDatastore function");
        }
        if (datastore.lastUpdate<NavSat>().has_value()) {
            bus.publish(*datastore.get<NavSat>());
        } else {
            logWarn("Datastore does not have data for type "
                    "NavSat in pollDatastore function");
        }
        if (datastore.lastUpdate<MonTx>().has_value()) {
            bus.publish(*datastore.get<MonTx>());
        } else {
            logWarn("Datastore does not have data for type "
                    "MonTx in pollDatastore function");
        }
        if (datastore.lastUpdate<MonRx>().has_value()) {
            bus.publish(*datastore.get<MonRx>());
        } else {
            logWarn("Datastore does not have data for type "
                    "MonRx in pollDatastore function");
        }
        if (datastore.lastUpdate<NavStatus>().has_value()) {
            bus.publish(*datastore.get<NavStatus>());
        } else {
            logWarn("Datastore does not have data for type "
                    "NavStatus in pollDatastore function");
        }
        if (datastore.lastUpdate<NavClock>().has_value()) {
            bus.publish(*datastore.get<NavClock>());
        } else {
            logWarn("Datastore does not have data for type "
                    "NavClock in pollDatastore function");
        }
        if (datastore.lastUpdate<UbxTimePulseStruct>().has_value()) {
            bus.publish(*datastore.get<UbxTimePulseStruct>());
        } else {
            logWarn("Datastore does not have data for type "
                    "UbxTimePulseStruct in pollDatastore function");
        }
        if (datastore.lastUpdate<GnssMonHwStruct>().has_value()) {
            bus.publish(*datastore.get<GnssMonHwStruct>());
        } else {
            logWarn("Datastore does not have data for type "
                    "GnssMonHwStruct in pollDatastore function");
        }
        if (datastore.lastUpdate<GnssMonHw2Struct>().has_value()) {
            bus.publish(*datastore.get<GnssMonHw2Struct>());
        } else {
            logWarn("Datastore does not have data for type "
                    "GnssMonHw2Struct in pollDatastore function");
        }
        if (datastore.lastUpdate<MqttStatusEvent>().has_value()) {
            bus.publish(*datastore.get<MqttStatusEvent>());
        } else {
            logWarn("Datastore does not have data for type "
                    "MqttStatusEvent in pollDatastore function");
        }
        if (datastore.lastUpdate<GpioInhibitEvent>().has_value()) {
            bus.publish(*datastore.get<GpioInhibitEvent>());
        } else {
            logWarn("Datastore does not have data for type "
                    "GpioInhibitEvent in pollDatastore function");
        }
        if (datastore.lastUpdate<LM75Event>().has_value()) {
            bus.publish(*datastore.get<LM75Event>());
        } else {
            logWarn("Datastore does not have data for type "
                    "LM75Event in pollDatastore function");
        }
        if (datastore.lastUpdate<PositionModeConfig>().has_value()) {
            bus.publish(*datastore.get<PositionModeConfig>());
        } else {
            logWarn("Datastore does not have data for type "
                    "PositionModeConfig in pollDatastore function");
        }
        if (datastore.lastUpdate<VersionEvent>().has_value()) {
            bus.publish(*datastore.get<VersionEvent>());
        } else {
            logWarn("Datastore does not have data for type "
                    "VersionEvent in pollDatastore function");
        }

        // NOT USED BY GUI:
        // if (datastore.lastUpdate<NavTimeGPS>().has_value()) {
        //     bus.publish(*datastore.get<NavTimeGPS>());
        // } else {
        //     logWarn("Datastore does not have data for type "
        //             "NavTimeGPS in pollDatastore function");
        // }
        // if (datastore.lastUpdate<NavTimeUTC>().has_value()) {
        //     bus.publish(*datastore.get<NavTimeUTC>());
        // } else {
        //     logWarn("Datastore does not have data for type "
        //             "NavTimeUTC in pollDatastore function");
        // }
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
    }

    inline static void pollAllUbxMsgRate(EventBus& bus) {
        for (auto msgID : rateCfgMsgID) {
            bus.publish(UbxMsgPollRateCmd{msgID});
        }
    }
    inline static void pollAllUbxMsg(EventBus& bus) {
        // Not rate configurable
        bus.publish(UbxMsgPollCmd{UBX_MSG::MON_VER});
        // Rate configurable:
        for (auto msgID : rateCfgMsgID) {
            bus.publish(UbxMsgPollCmd{msgID});
        }
    }

  private:
    EventBindings() = delete;
};
#endif // EVENT_BINDINGS_H