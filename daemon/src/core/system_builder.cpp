#include "core/system_builder.h"

#include "app/config_parser.h"
#include "app/system_config.h"
#include "core/logging/logger.h"
#include "core/scheduler.h"
#include "core/thread_pool.h"

// Configurations
#include "core/component_config.h"
#include "core/device_config.h"

// Factories
#include "core/factories/component_factory.h"
#include "core/factories/device_factory.h"
#include "core/factories/sink_factory.h"

// Registries
#include "core/event_bus.h"
#include "core/registries/component_manager.h"
#include "core/registries/data_store.h"
#include "core/registries/device_registry.h"
#include "core/registries/sink_manager.h"
#include "network/tcpserver.h"

// Devices
#include "core/component.h"
#include "drivers/eeprom24aa02_driver.h"
#include "drivers/gpio_driver.h"
#include "hardware/devices.h"
#include "hardware/i2cdevice_wrapper.h"
#include "hardware/i2cdevices.h"
#include "hardware/ublox/serialublox.h"

// Ublox
#include "data/commands/histogram_clear_cmd.h"
#include "data/commands/ubx_dynamic_model_cmd.h"
#include "data/commands/ubx_msg_poll_cmd.h"
#include "data/commands/ubx_msg_rate_cmd.h"
#include "data/commands/ubx_protocol_selection_cmd.h"
#include "data/commands/ubx_rate_cmd.h"
#include "data/commands/ubx_set_aop_cmd.h"
#include "data/commands/ubx_version_dependent_cmd.h"
#include "data/ublox/ublox_messages.h"

// Sources
#include "sources/source.h"
#include "sources/tcp_source.h"

// Data
#include "data/events/ads1115_event.h"
#include "data/events/event_trigger_event.h"
#include "data/events/gpio_event.h"
#include "data/events/log_trigger_event.h"
#include "data/events/server_conn_count_event.h"
#include "data/events/status_led_event.h"
#include "utility/logparameter.h"
// #include "data/events/ubx_event.h"
// #include "data/events/tcp_packet_event.h"

// Commands
#include "data/commands/bias_switch_cmd.h"
#include "data/commands/bias_voltage_cmd.h"
#include "data/commands/gpio_signal_set_cmd.h"
#include "data/commands/pca_switch_cmd.h"

// Glue
#include "core/event_bindings.h"

#include <chrono>
#include <memory>

Context SystemBuilder::build(ThreadPool& pool, const SystemConfig& config) {
    // First try loading libconfig data

    std::vector<DeviceConfig> deviceConfigurations =
        SystemBuilder::loadHardwareConfig(config.hardwareConfigPath);
    std::vector<ComponentConfig> componentConfigurations =
        SystemBuilder::loadComponentConfig(config.componentConfigPath);

    // Build System
    Context ctx;

    ctx.io = std::make_shared<boost::asio::io_context>();
    ctx.registry = std::make_unique<DeviceRegistry>();
    ctx.components = std::make_unique<ComponentManager>();
    ctx.sinks = std::make_unique<SinkManager>();
    ctx.datastore = std::make_unique<DataStore>();
    ctx.bus = std::make_unique<EventBus>(pool);
    ctx.scheduler = std::make_unique<Scheduler>(pool);
    ctx.config = std::make_unique<SystemConfig>(std::move(config));

    // --- datastore ---
    EventBindings::setupDatastore(*ctx.bus, *ctx.datastore);

    // ctx.bus->subscribe<>(

    // )
    // sendHistogram(*m_histo_map[histoName]);

    // --- hardware ---
    DeviceFactory::i2cReset();
    for (auto& d : deviceConfigurations) {
        ctx.registry->add(d.id, DeviceFactory::create(d));
    }

    // --- components ---
    for (auto& c : componentConfigurations) {
        auto component = ComponentFactory::createComponent(c, ctx);
        logDebug("Created component " + component->name());
        auto component_as_source = std::dynamic_pointer_cast<Source>(component);
        if (component_as_source) {
            component_as_source->update(); // Read out data once in the beginning!
            std::weak_ptr<Source> weak = component_as_source;
            if (c.interval) {
                ctx.scheduler->every(c.interval.value(), [weak]() {
                    if (auto locked = weak.lock()) {
                        locked->update();
                    }
                });
            }
        }

        // Store component in component manager
        ctx.components->add(component->id(), component);
    }

    // --- Calibration from EEPROM ---
    auto eeprom = ctx.components->get<EEPROM24AA02Driver>(Device::EEPROM24AA02_0);
    if (eeprom != nullptr) {
        eeprom->update();
    } // -> Sets MuonPi::Version::hardware
    else {
        logWarn("EEPROM not initializing.");
    }

    // --- GPIO Initialization ---
    auto gpio_driver = ctx.components->get<GpioDriver>(OtherComponent::GPIO_DRIVER_0);
    if (gpio_driver != nullptr) {
        gpio_driver->init(MuonPi::Version::hardware);

        // Setup status LED timing
        ctx.bus->subscribe<StatusLedEvent>(
            [gpio_driver, &scheduler = *ctx.scheduler](const StatusLedEvent& event) {
                gpio_driver->writeSignal(event.sig, event.on);
                if (event.durationMillisec >= 0) {
                    scheduler.once([gpio_driver,
                                    event]() { gpio_driver->writeSignal(event.sig, !(event.on)); },
                                   static_cast<std::size_t>(event.durationMillisec));
                }
            });

        // every once in a while send gpio average rates to GUI
        ctx.scheduler->every(std::chrono::seconds(1),
                             [gpio_driver]() { gpio_driver->sendGpioRatesAverage(); });
    } else {
        logWarn("GPIO Driver not initializing.");
    }

    // TODO: Replace this call with Sink Factory for all sinks
    // --- sinks ---
    auto tcp_sink = SinkFactory::createTcpSink(ctx.sinks);
    EventBindings::setupTcpSink(*ctx.bus, *tcp_sink);

    // --- tcp_server ---
    // When server accepts a new TCP connection, call this handler.
    ctx.server = std::make_unique<TcpServer>(ctx.io, config.serverPort, tcp_sink, ctx.bus.get());

    ctx.server->addConnectionHandler(
        [tcp_source = ctx.components->get<TcpSource>(OtherComponent::TCP_SOURCE_0),
         &bus = ctx.bus](const std::shared_ptr<TcpConnection>& connection) {
            if (tcp_source != nullptr) {
                tcp_source->registerConnection(connection);
                EventBindings::pollAllUbxMsgRate(*bus);
            } else {
                logError("Nullpointer in creating connection handler for tcpsource. Make sure "
                         "components are initialized "
                         "beforehand.");
            }
        });

    // --- maintenance ---
    ctx.scheduler->every(std::chrono::seconds(5), [server = ctx.server.get()]() {
        server->heartbeatAndCleanup(std::chrono::seconds(30));
    });

    ctx.scheduler->every(MuonPi::Config::Log::interval,
                         [bus = ctx.bus.get()]() { bus->publish(LogTriggerEvent{}); });

    ctx.scheduler->every(MuonPi::Config::Histogram::interval,
                         [bus = ctx.bus.get()]() { bus->publish(HistogramRequestCmd{}); });

    // --- GPS default config ---
    ctx.bus->subscribe<UbxConfigDefaultCmd>(
        [&bus = *ctx.bus, &config = *ctx.config]([[maybe_unused]] const auto&) {
            bus.publish(UbxProtocolSelectionCmd{1, PROTO_UBX});
            logInfo("setting GNSS dynamic model to " +
                    std::to_string(static_cast<unsigned>(config.gnss_dynamic_model)));
            bus.publish(config.gnss_dynamic_model);
            bus.publish(UbxSetAopCmd{true});
            bus.publish(UbxMsgPollCmd{UBX_MSG::MON_VER});
            std::uint16_t measinterval = 100;
            bus.publish(UbxRateCmd{measinterval, 1}); // set rate of messages and nav
            // --- Message Rates ---
            EventBindings::initAllUbxMsgRate(bus);
            // Enable NAV_SVINFO only for version 0.1 - 15.0
            // Enable NAV_SAT for version > 15.0
            bus.publish(UbxVersionDependentCmd{
                {UbxVersionDependentCmd::Entry{Version{0, 1}, Version{15, 0},
                                               UbxMsgRateCmd{UBX_MSG::NAV_SVINFO, 1, 69}},
                 UbxVersionDependentCmd::Entry{Version{15, 0},
                                               Version{std::numeric_limits<unsigned>().max(), 0},
                                               UbxMsgRateCmd{UBX_MSG::NAV_SVINFO, 1, 0}}}});
            bus.publish(UbxVersionDependentCmd{{UbxVersionDependentCmd::Entry{
                Version{15, 0}, Version{std::numeric_limits<unsigned>().max(), 0},
                UbxMsgRateCmd{UBX_MSG::NAV_SAT, 1, 69}}}});
            EventBindings::pollAllUbxMsgRate(bus);
        });
    ctx.bus->publish(UbxConfigDefaultCmd{}); // Apply default config

    // Debug reading version every second // TESTED -> Working!
    // ctx.scheduler->every(std::chrono::milliseconds(1000),[bus = &(*ctx.bus)](){
    //     bus->publish(UbxMsgPollCmd{UBX_MSG::MON_VER});
    // });

    // every once in a while, send data to OLED
    // ctx.scheduler->every(std::chrono::milliseconds(MuonPi::Config::Hardware::OLED::update_interval),[&ctx](){
    // ctx.bus->publish(...);
    // });

    // set up cyclic timer monitoring following operational parameters:
    // temp, vadc, vbias, ibias
    // ctx.scheduler->every(std::chrono::milliseconds(Config::Hardware::monitor_interval),[&ctx](){
    // ctx.bus->publish(...);
    // });

    ctx.bus->publish<LogParameter>(LogParameter(
        "maxGeohashLength", std::to_string(config.maxGeohashLength), LogParameter::LOG_ONCE));
    ctx.bus->publish<LogParameter>(LogParameter(
        "softwareVersionString", MuonPi::Version::software.string(), LogParameter::LOG_ONCE));
    ctx.bus->publish<LogParameter>(LogParameter(
        "hardwareVersionString", MuonPi::Version::hardware.string(), LogParameter::LOG_ONCE));

    //     m_geopos_manager.set_lockin_ready_callback(std::bind(&Daemon::onGeoPosLockInReady, this,
    //     std::placeholders::_1));
    // m_geopos_manager.set_valid_pos_callback(std::bind(&Daemon::onGeoPosValid, this,
    // std::placeholders::_1)); m_geopos_manager.set_mode_config(config.position_mode_config);

    // --- Set Gpio Output ---
    ctx.bus->publish<GpioSignalSetCmd>({UBIAS_EN, true});
    ctx.bus->publish<GpioSignalSetCmd>({PREAMP_1, config.preamp_enable[0]});
    ctx.bus->publish<GpioSignalSetCmd>({PREAMP_2, config.preamp_enable[1]});
    ctx.bus->publish<GpioSignalSetCmd>({GAIN_HL, config.hi_gain});
    ctx.bus->publish<GpioSignalSetCmd>({STATUS1, false});
    ctx.bus->publish<GpioSignalSetCmd>({STATUS2, false});
    ctx.bus->publish<GpioSignalSetCmd>({IN_POL1, config.polarity[0]});
    ctx.bus->publish<GpioSignalSetCmd>({IN_POL2, config.polarity[1]});

    // -- Behaviour on new tcp connection ---
    ctx.bus->subscribe<ServerConnCountEvent>([&bus = *ctx.bus, &config = *ctx.config,
                                              &datastore = *ctx.datastore]([[maybe_unused]] auto&) {
        bus.publish(
            VersionEvent{.hw_ver = MuonPi::Version::hardware, .sw_ver = MuonPi::Version::software});
        // Where there is already some event for requesting, do so via event bus
        bus.publish(BiasSwitchRequestCmd{});
        bus.publish(DacSettingRequestCmd{});
        bus.publish(PcaSwitchRequestCmd{});
        bus.publish(EventTriggerRequestCmd{});
        // Where there is no such thing, request through datastore
        const auto& mode = datastore.geoPosManager().get_mode_config();
        bus.publish(mode);
        if (mode.mode == PositionModeConfig::Mode::Static) {
            bus.publish(datastore.geoPosManager().get_static_position().getPosStruct());
        }
    });

    return ctx;
}

/// LOADING OF CONFIG FILES & PARSING OF CONFIG

auto SystemBuilder::loadHardwareConfig(const std::string& hardwareConfigPath)
    -> std::vector<DeviceConfig> {

    std::shared_ptr<libconfig::Config> hardwareConfig{nullptr};

    try {
        hardwareConfig = ConfigParser::loadConfigFile(hardwareConfigPath);
    } catch (const libconfig::FileIOException& fioex) {
        logError("Error while reading config file " + hardwareConfigPath);
    } catch (const libconfig::ParseException& pex) {
        logError("Parse error at " + std::string(pex.getFile()) + " : line " +
                 std::to_string(pex.getLine()) + " - " + std::string(pex.getError()));
        throw pex;
    }

    try {
        return SystemBuilder::parseHardwareConfig(*hardwareConfig);
    } catch (const libconfig::SettingNotFoundException& e) {
        logWarn(std::string(e.what()) + " in file " + hardwareConfigPath + " " +
                std::string(e.getPath()));
    } catch (const libconfig::SettingException& e) {
        logWarn(std::string(e.what()) + " in file " + hardwareConfigPath + " " +
                std::string(e.getPath()));
    }
    throw std::runtime_error("Could not parse hardware configuration");
}

auto SystemBuilder::parseHardwareConfig(const libconfig::Config& hardwareConfig)
    -> std::vector<DeviceConfig> {

    std::vector<DeviceConfig> result;

    const libconfig::Setting& root = hardwareConfig.getRoot();
    const libconfig::Setting& devices = root.lookup("devices");
    const int count = devices.getLength();

    std::string deviceList{};
    for (const auto& [key, value] : deviceLookup) {
        deviceList.append(key + "\n");
    }

    std::map<std::string, DeviceType> deviceTypeLookup;

    for (const auto& [type, name] : DeviceTypes) {
        deviceTypeLookup.emplace(name, type);
    }

    for (int i = 0; i < count; ++i) {
        const libconfig::Setting& d = devices[i];

        DeviceConfig cfg;

        std::string idStr;
        if (d.lookupValue("id", idStr) == false) {
            logWarn("id not found in hardware configuration at device " + std::to_string(i));
            continue;
        }

        auto id = deviceLookup.find(idStr);
        if (id == deviceLookup.end()) {
            logWarn("Device " + idStr + " not found in configurable devices, devices are:\n" +
                    deviceList);
            continue;
        }
        cfg.id = id->second;

        std::string typeStr;
        if (d.lookupValue("type", typeStr) == false) {
            continue;
        }
        auto type_it = deviceTypeLookup.find(typeStr);
        if (type_it != deviceTypeLookup.end()) {
            cfg.type = type_it->second;
        }

        if (d.lookupValue("category", cfg.category) == false) {
            continue;
        }
        if (cfg.category == "i2c") {
            std::string dev;
            int address;

            d.lookupValue("device", dev);
            d.lookupValue("address", address);

            if (d.lookupValue("device", dev) != false) {
                cfg.device = dev;
            }
            if (d.lookupValue("address", address) != false) {
                if (address < 0 || address > std::numeric_limits<std::uint8_t>::max()) {
                    logError("Hardware address parsing error, outside uint8_t range. Ignoring "
                             "address setting for device " +
                             idStr);
                } else {
                    cfg.address = static_cast<std::uint8_t>(address);
                }
            }
        } else if (cfg.category == "uart") {
            std::string dev;
            int baud;

            if (d.lookupValue("device", dev) != false) {
                cfg.device = dev;
            }
            if (d.lookupValue("baud", baud) != false) {
                if (baud < 0) {
                    logError("Baudrate parsing error, outside uint32_t range. Ignoring setting for "
                             "device " +
                             idStr);
                }
                cfg.baud = baud;
            }
        }

        result.push_back(cfg);
    }
    return result;
}

auto SystemBuilder::loadComponentConfig(const std::string& componentConfigPath)
    -> std::vector<ComponentConfig> {

    std::shared_ptr<libconfig::Config> componentConfig{nullptr};
    try {
        componentConfig = ConfigParser::loadConfigFile(componentConfigPath);
    } catch (const libconfig::FileIOException& fioex) {
        logError("Error while reading config file " + componentConfigPath);
    } catch (const libconfig::ParseException& pex) {
        logError("Parse error at " + std::string(pex.getFile()) + " : line " +
                 std::to_string(pex.getLine()) + " - " + std::string(pex.getError()));
        throw pex;
    }

    try {
        return SystemBuilder::parseComponentConfig(*componentConfig);
    } catch (const libconfig::SettingNotFoundException& e) {
        logWarn(std::string(e.what()) + " in file " + componentConfigPath + " " +
                std::string(e.getPath()));
    } catch (const libconfig::SettingException& e) {
        logWarn(std::string(e.what()) + " in file " + componentConfigPath + " " +
                std::string(e.getPath()));
    }
    throw std::runtime_error("Could not parse component configuration");
}

auto SystemBuilder::parseComponentConfig(const libconfig::Config& componentConfig)
    -> std::vector<ComponentConfig> {
    std::vector<ComponentConfig> result;

    const libconfig::Setting& root = componentConfig.getRoot();
    const libconfig::Setting& components = root.lookup("components");
    const int count = components.getLength();

    for (int i = 0; i < count; ++i) {
        ComponentConfig cfg;
        const libconfig::Setting& s = components[i];

        std::string idStr;
        if (s.lookupValue("id", idStr) == false) {
            logWarn("id not found in components configuration at source " + std::to_string(i));
            continue;
        }
        auto id = componentLookup.find(idStr);
        if (id == componentLookup.end()) {
            logWarn("Source '" + idStr + "' not found.");
            continue;
        }
        cfg.id = ComponentId{id->second};

        std::string deviceIdStr;
        if (s.lookupValue("device", deviceIdStr) != false) {
            auto deviceId = deviceLookup.find(deviceIdStr);
            if (deviceId != deviceLookup.end()) {
                cfg.deviceId = deviceId->second;
            }
        }
        result.push_back(cfg);
    }
    return result;
}