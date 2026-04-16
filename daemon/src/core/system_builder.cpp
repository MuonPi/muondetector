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
#include "core/registries/device_registry.h"
#include "core/registries/sink_manager.h"
#include "network/tcpserver.h"

// Devices
#include "hardware/devices.h"
#include "hardware/i2cdevice_wrapper.h"
#include "hardware/i2cdevices.h"

#include "core/component.h"

// Sources
#include "sources/source.h"
#include "sources/tcp_source.h"

// Data
#include "data/events/ad1115_event.h"
// #include "data/events/ubx_event.h"
// #include "data/events/tcp_packet_event.h"

#include <chrono>
#include <memory>

auto SystemBuilder::parseHardwareConfig(const libconfig::Config &hardwareConfig)
    -> std::vector<DeviceConfig>
{
    std::vector<DeviceConfig> result;

    const libconfig::Setting &root = hardwareConfig.getRoot();
    const libconfig::Setting &devices = root.lookup("devices");
    const int count = devices.getLength();

    std::string deviceList{};
    for (const auto &[key, value] : deviceLookup)
    {
        deviceList.append(key + "\n");
    }

    std::map<std::string, DeviceType> deviceTypeLookup;

    for (const auto &[type, name] : DeviceTypes)
    {
        deviceTypeLookup.emplace(name, type);
    }

    for (int i = 0; i < count; ++i)
    {
        const libconfig::Setting &d = devices[i];

        DeviceConfig cfg;

        std::string idStr;
        if (d.lookupValue("id", idStr) == false)
        {
            logWarn("id not found in hardware configuration at device " + std::to_string(i));
            continue;
        }

        auto id = deviceLookup.find(idStr);
        if (id == deviceLookup.end())
        {
            logWarn("Device " + idStr + " not found in configurable devices, devices are:\n" + deviceList);
            continue;
        }
        cfg.id = id->second;

        std::string typeStr;
        if (d.lookupValue("type", typeStr) == false)
        {
            continue;
        }
        auto type_it = deviceTypeLookup.find(typeStr);
        if (type_it != deviceTypeLookup.end())
        {
            cfg.type = type_it->second;
        }

        if (d.lookupValue("category", cfg.category) == false)
        {
            continue;
        }
        if (cfg.category == "i2c")
        {
            std::string dev;
            int address;

            d.lookupValue("device", dev);
            d.lookupValue("address", address);

            if (d.lookupValue("device", dev) != false)
            {
                cfg.device = dev;
            }
            if (d.lookupValue("address", address) != false)
            {
                if (address < 0 || address > std::numeric_limits<std::uint8_t>::max())
                {
                    logError(
                        "Hardware address parsing error, outside uint8_t range. Ignoring address setting for device " +
                        idStr);
                }
                else
                {
                    cfg.address = static_cast<std::uint8_t>(address);
                }
            }
        }
        else if (cfg.category == "uart")
        {
            std::string dev;
            int baud;

            if (d.lookupValue("device", dev) != false)
            {
                cfg.device = dev;
            }
            if (d.lookupValue("baud", baud) != false)
            {
                if (baud > 0 || baud > std::numeric_limits<std::uint32_t>::max())
                {
                    logError("Baudrate parsing error, outside uint32_t range. Ignoring setting for device " + idStr);
                }
                cfg.baud = baud;
            }
        }

        result.push_back(cfg);
    }
    return result;
}

auto SystemBuilder::parseComponentConfig(const libconfig::Config &componentConfig)
    -> std::vector<ComponentConfig>
{
    std::vector<ComponentConfig> result;

    const libconfig::Setting &root = componentConfig.getRoot();
    const libconfig::Setting &components = root.lookup("components");
    const int count = components.getLength();

    for (int i = 0; i < count; ++i)
    {
        ComponentConfig cfg;
        const libconfig::Setting &s = components[i];

        std::string idStr;
        if (s.lookupValue("id", idStr) == false)
        {
            logWarn("id not found in components configuration at source " + std::to_string(i));
            continue;
        }
        auto id = componentLookup.find(idStr);
        if (id == componentLookup.end())
        {
            logWarn("Source '" + idStr + "' not found.");
            continue;
        }
        cfg.id = ComponentId{id->second};

        std::string deviceIdStr;
        if (s.lookupValue("device", deviceIdStr) != false)
        {
            auto deviceId = deviceLookup.find(deviceIdStr);
            if (deviceId != deviceLookup.end())
            {
                cfg.deviceId = deviceId->second;
            }
        }
        result.push_back(cfg);
    }
    return result;
}

Context SystemBuilder::build(ThreadPool &pool, const SystemConfig &config)
{
    // First try loading libconfig data
    std::shared_ptr<libconfig::Config> hardwareConfig{nullptr};
    std::shared_ptr<libconfig::Config> componentConfig{nullptr};
    try
    {
        hardwareConfig = ConfigParser::loadConfigFile(config.hardwareConfigPath);
    }
    catch (const libconfig::FileIOException &fioex)
    {
        logError("Error while reading config file " + config.hardwareConfigPath);
    }
    catch (const libconfig::ParseException &pex)
    {
        logError("Parse error at " + std::string(pex.getFile()) + " : line " + std::to_string(pex.getLine()) + " - " +
                 std::string(pex.getError()));
        throw pex;
    }
    try
    {
        componentConfig = ConfigParser::loadConfigFile(config.componentConfigPath);
    }
    catch (const libconfig::FileIOException &fioex)
    {
        logError("Error while reading config file " + config.componentConfigPath);
    }
    catch (const libconfig::ParseException &pex)
    {
        logError("Parse error at " + std::string(pex.getFile()) + " : line " + std::to_string(pex.getLine()) + " - " +
                 std::string(pex.getError()));
        throw pex;
    }

    // Build System
    Context ctx;

    ctx.io = std::make_shared<boost::asio::io_context>();
    ctx.registry = std::make_unique<DeviceRegistry>();
    ctx.components = std::make_unique<ComponentManager>();
    ctx.sinks = std::make_unique<SinkManager>();
    ctx.bus = std::make_unique<EventBus>(pool);
    ctx.scheduler = std::make_unique<Scheduler>(pool);
    ctx.config = std::make_unique<SystemConfig>(std::move(config));

    std::vector<DeviceConfig> deviceConfigurations;
    try
    {
        deviceConfigurations = SystemBuilder::parseHardwareConfig(*hardwareConfig);
    }
    catch (const libconfig::SettingNotFoundException &e)
    {
        logWarn(std::string(e.what()) + " in file " + config.hardwareConfigPath + " " + std::string(e.getPath()));
    }
    catch (const libconfig::SettingException &e)
    {
        logWarn(std::string(e.what()) + " in file " + config.hardwareConfigPath + " " + std::string(e.getPath()));
    }
    std::vector<ComponentConfig> componentConfigurations;
    try
    {
        componentConfigurations = SystemBuilder::parseComponentConfig(*componentConfig);
    }
    catch (const libconfig::SettingNotFoundException &e)
    {
        logWarn(std::string(e.what()) + " in file " + config.componentConfigPath + " " + std::string(e.getPath()));
    }
    catch (const libconfig::SettingException &e)
    {
        logWarn(std::string(e.what()) + " in file " + config.componentConfigPath + " " + std::string(e.getPath()));
    }

    // --- hardware ---
    for (auto &d : deviceConfigurations)
    {
        ctx.registry->add(d.id, DeviceFactory::create(d));
    }

    // --- components ---
    for (auto &c : componentConfigurations)
    {
        auto component = ComponentFactory::createComponent(c, ctx);
        auto component_as_source = std::dynamic_pointer_cast<Source>(component);
        if (component_as_source)
        {
            std::weak_ptr<Source> weak = component_as_source;
            if (c.interval)
            {
                ctx.scheduler->every(c.interval.value(), [weak]() {
                    if (auto locked = weak.lock())
                    {
                        locked->update();
                    }
                });
            }
        }

        // Store component in component manager
        ctx.components->add(component->id(), component);
    }

    // --- sinks ---
    auto tcp_sink = SinkFactory::createTcpSink(ctx.sinks);

    // make tcp sink send data through tcp connections
    ctx.bus->subscribe<Ads1115Event>(std::bind(&TcpSink::handle, tcp_sink, std::placeholders::_1));

    // --- tcp_server ---
    // When server accepts a new TCP connection, call this handler.
    ctx.server = std::make_unique<TcpServer>(ctx.io, config.serverPort, tcp_sink);

    ctx.server->addConnectionHandler([tcp_source = ctx.components->get<TcpSource>(NonDeviceComponent::TCP_SOURCE_0)](
                                         const std::shared_ptr<TcpConnection> &connection) {
        if (tcp_source != nullptr)
        {
            tcp_source->registerConnection(connection);
        }
        else
        {
            logError("Nullpointer in creating connection handler for tcpsource. Make sure components are initialized "
                     "beforehand.");
        }
    });

    // --- maintenance ---
    ctx.scheduler->every(std::chrono::seconds(5),
                         [server = ctx.server.get()]() { server->heartbeatAndCleanup(std::chrono::seconds(30)); });

    return ctx;
}
