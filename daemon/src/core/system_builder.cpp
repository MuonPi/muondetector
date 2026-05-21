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

// Ublox
#include "data/commands/histogram_clear_cmd.h"
#include "data/ublox/ublox_messages.h"

// Sources
#include "sources/source.h"
#include "sources/tcp_source.h"

// Data
#include "data/events/ads1115_event.h"
#include "data/events/event_trigger_event.h"
#include "data/events/gpio_event.h"
#include "data/events/gpio_inhibit_event.h"
#include "data/events/log_trigger_event.h"
#include "data/events/mqtt_status_event.h"
#include "data/events/server_conn_count_event.h"
#include "data/events/status_led_event.h"
#include "data/events/version_event.h"
#include "utility/logparameter.h"

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
    ctx.datastore->geoPosManager().set_mode_config(ctx.config->position_mode_config);
    ctx.datastore->store(ctx.config->position_mode_config);
    ctx.datastore->store(
        VersionEvent{.hw_ver = MuonPi::Version::hardware, .sw_ver = MuonPi::Version::software});
    ctx.datastore->store(EventTriggerEvent{.signal = ctx.config->eventTrigger});
    ctx.datastore->store(GpioInhibitEvent{.inhibit = false});
    ctx.datastore->store(MqttStatusEvent{.connected = false});

    // --- hardware ---
    DeviceFactory::i2cReset();
    for (auto& d : deviceConfigurations) {
        ctx.registry->add(d.id, DeviceFactory::create(d));
    }

    // Make sure I2C Stats are emitted correctly
    ctx.bus->subscribe<I2CStatsRequestCmd>([&bus = *ctx.bus]([[maybe_unused]] const auto&) {
        I2CStatsEvent event{.nrDevices =
                                static_cast<std::uint8_t>(i2cDevice::getGlobalDeviceList().size()),
                            .bytesRead = i2cDevice::getGlobalNrBytesRead(),
                            .bytesWritten = i2cDevice::getGlobalNrBytesWritten()};
        event.deviceList.reserve(i2cDevice::getGlobalDeviceList().size());

        for (std::size_t i = 0; i < i2cDevice::getGlobalDeviceList().size(); i++) {
            I2cDeviceEntry entry{
                .address = i2cDevice::getGlobalDeviceList()[i]->getAddress(),
                .name = i2cDevice::getGlobalDeviceList()[i]->getTitle(),
                .status = i2cDevice::getGlobalDeviceList()[i]->getStatus(),
                .nrBytesWritten = i2cDevice::getGlobalDeviceList()[i]->getNrBytesWritten(),
                .nrBytesRead = i2cDevice::getGlobalDeviceList()[i]->getNrBytesRead(),
                .nrIoErrors = i2cDevice::getGlobalDeviceList()[i]->getNrIOErrors(),
                .lastTransactionTime = static_cast<std::uint32_t>(
                    i2cDevice::getGlobalDeviceList()[i]->getLastTimeInterval())};
            event.deviceList.push_back(std::move(entry));
        }
        bus.publish(std::move(event));
    });

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

        // Setup status LED timing -> after some time the status led should turn off
        ctx.bus->subscribe<StatusLedEvent>(
            [gpio_driver, &scheduler = *ctx.scheduler](const StatusLedEvent& event) {
                if (event.on == false) {
                    return;
                }
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
         &bus = *ctx.bus](const std::shared_ptr<TcpConnection>& connection) {
            if (tcp_source != nullptr) {
                tcp_source->registerConnection(connection);
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

    // --- Set Gpio Output ---
    ctx.bus->publish(BiasSwitchCmd{true});
    ctx.bus->publish(PreampSwitchCmd{0, config.preamp_enable[0]});
    ctx.bus->publish(PreampSwitchCmd{1, config.preamp_enable[1]});
    ctx.bus->publish(GainSwitchCmd{config.hi_gain});
    ctx.bus->publish(GpioSignalSetCmd{STATUS1, false});
    ctx.bus->publish(GpioSignalSetCmd{STATUS2, false});
    ctx.bus->publish(EventTriggerCmd{config.eventTrigger});

    // --- Set other config defaults ---
    ctx.bus->publish(PolaritySwitchCmd{.pol1 = config.polarity[0], .pol2 = config.polarity[1]});

    // -- Behaviour on new tcp connection ---
    ctx.bus->subscribe<ServerConnCountEvent>(
        [&bus = *ctx.bus, &config = *ctx.config, &datastore = *ctx.datastore](const auto& event) {
            if (event.newlyConnected == false) {
                // no need to send new data
                return;
            }
            // Where there is already some event for requesting, do so via event bus
            EventBindings::pollDatastore(bus, datastore);
            // Where there is no such thing, request through datastore
            const auto& mode = datastore.geoPosManager().get_mode_config();
            if (mode.mode == PositionModeConfig::Mode::Static) {
                bus.publish(datastore.geoPosManager().get_static_position().getPosStruct());
            }
        });

    return ctx;

    // Trigger Ublox device configuration
    ctx.bus->publish(UbxConfigDefaultCmd{});
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
