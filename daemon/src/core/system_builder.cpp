#include "core/system_builder.h"
#include "app/system_config.h"
#include "app/config_parser.h"
#include "core/thread_pool.h"
#include "core/scheduler.h"
#include "core/logging/logger.h"

// Configurations
#include "core/source_config.h"
#include "core/device_config.h"

// Factories
#include "core/factories/device_factory.h"
#include "core/factories/source_factory.h"
#include "core/factories/sink_factory.h"

// Registries
#include "core/registries/device_registry.h"
#include "core/registries/source_manager.h"
#include "core/registries/sink_manager.h"
#include "core/event_bus.h"
#include "network/tcpserver.h"

// Devices
#include "hardware/devices.h"
#include "hardware/i2cdevices.h"
#include "hardware/i2cdevice_wrapper.h"

// Sources
#include "sources/tcp_source.h"

// Data
#include "data/ad1115_event.h"

#include <memory>
#include <chrono>

auto SystemBuilder::parseHardwareConfig(Context& ctx, const libconfig::Config& hardwareConfig) -> std::vector<DeviceConfig>
{
    std::vector<DeviceConfig> result;

    const libconfig::Setting& root = hardwareConfig.getRoot();
    const libconfig::Setting& devices = root.lookup("devices");
    const int count = devices.getLength();

    std::string deviceList{};
    for ( const auto &[key, value]: deviceLookup ) {
        deviceList.append(key + "\n");
    }


    std::map<std::string, DeviceType> deviceTypeLookup;

    for (const auto& [type, name] : DeviceTypes)
    {
        deviceTypeLookup.emplace(name, type);
    }

    for (int i = 0; i < count; ++i)
    {
        const libconfig::Setting& d = devices[i];

        DeviceConfig cfg;

        std::string idStr;
        if (d.lookupValue("id", idStr) == false) {
            logWarn("id not found in hardware configuration at device " + std::to_string(i));
            continue;
        }

        auto id = deviceLookup.find(idStr);
        if (id == deviceLookup.end()) {
            logWarn("Device " + idStr + " not found in configurable devices, devices are:\n" + deviceList);
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
        if (cfg.category == "i2c")
        {
            std::string dev;
            int address;

            d.lookupValue("device", dev);
            d.lookupValue("address", address);

            if (d.lookupValue("device", dev) != false) {
                cfg.device = dev;
            }
            if (d.lookupValue("address", address) != false) {
                if (address < 0 || address > std::numeric_limits<std::uint8_t>::max()) {
                    logError("Hardware address parsing error, outside uint8_t range. Ignoring address setting for device " + idStr);
                } else {
                    cfg.address = static_cast<std::uint8_t>(address);
                }
            }
        }
        else if (cfg.category == "uart")
        {
            std::string dev;
            int baud;

            if (d.lookupValue("device", dev) != false) {
                cfg.device = dev;
            }
            if (d.lookupValue("baud", baud) != false) {
                if (baud > 0 || baud > std::numeric_limits<std::uint32_t>::max()) {
                    logError("Baudrate parsing error, outside uint32_t range. Ignoring setting for device " + idStr);
                }
                cfg.baud = baud;
            }
        }

        result.push_back(cfg);
    }
    return result;
}

auto SystemBuilder::parseSourcesConfig(Context& ctx, const libconfig::Config& sourcesConfig) -> std::vector<SourceConfig>
{
    std::vector<SourceConfig> result;

    const libconfig::Setting& root = sourcesConfig.getRoot();
    const libconfig::Setting& sources = root.lookup("sources");
    const int count = sources.getLength();


    for (int i = 0; i < count; ++i)
    {
        SourceConfig cfg;
        const libconfig::Setting& s = sources[i];

        std::string idStr;
        if (s.lookupValue("id", idStr) == false) {
            logWarn("id not found in sources configuration at source " + std::to_string(i));
            continue;
        }
        auto id = sourceLookup.find(idStr);
        if (id == sourceLookup.end()) {
            logWarn("Source '" + idStr + "' not found.");
            continue;
        }
        cfg.id = SourceId{id->second};

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

SystemBuilder::Context SystemBuilder::build(ThreadPool& pool, const SystemConfig& config)
{
    // First try loading libconfig data
    std::shared_ptr<libconfig::Config> hardwareConfig{nullptr};
    std::shared_ptr<libconfig::Config> sourcesConfig{nullptr};
    try {
        hardwareConfig = ConfigParser::loadConfigFile(config.hardwareConfigPath);
    } catch (const libconfig::ParseException& e) {
        std::stringstream sstr{};
        sstr << e.getFile() << ":" << e.getLine()
              << " - " << e.getError() << std::endl;
        throw std::runtime_error(sstr.str());
    }
    try {
        sourcesConfig = ConfigParser::loadConfigFile(config.sourcesConfigPath);
    } catch (const libconfig::ParseException& e) {
        std::stringstream sstr{};
        sstr << e.getFile() << ":" << e.getLine()
              << " - " << e.getError() << std::endl;
        throw std::runtime_error(sstr.str());
    }

    // Build System
    Context ctx;

    ctx.io = std::make_shared<boost::asio::io_context>();
    ctx.registry = std::make_unique<DeviceRegistry>();
    ctx.sources = std::make_unique<SourceManager>();
    ctx.sinks = std::make_unique<SinkManager>();
    ctx.bus = std::make_unique<EventBus>(pool);
    ctx.scheduler = std::make_unique<Scheduler>(pool);

    std::vector<DeviceConfig> deviceConfigurations = SystemBuilder::parseHardwareConfig(ctx, *hardwareConfig);
    std::vector<SourceConfig> sourceConfigurations = SystemBuilder::parseSourcesConfig(ctx, *sourcesConfig);


    // --- hardware ---
    for (auto& devConfig : deviceConfigurations) {
        ctx.registry->add(devConfig.id, DeviceFactory::create(devConfig));
    }

    // --- sources ---
    for (auto& sourceConfig : sourceConfigurations) {
        auto source = SourceFactory::createSource(sourceConfig, *ctx.registry, *ctx.bus);
        ctx.sources->add(source->id(), source);
        if (sourceConfig.interval.has_value()) {
            ctx.scheduler->every(sourceConfig.interval.value(), [source]() {
                source->update();
            });
        }
    }

    // --- sinks ---
    auto tcp_sink = SinkFactory::createTcpSink(ctx.sinks);


    // make tcp sink send data through tcp connections
    ctx.bus->subscribe<Ads1115Event>(std::bind(&TcpSink::handle, tcp_sink, std::placeholders::_1));

    // --- tcp_server ---
    // When server accepts a new TCP connection, call this handler.
    ctx.server = std::make_unique<TcpServer>(ctx.io, config.serverPort, tcp_sink);

    ctx.server->addConnectionHandler([tcp_source = ctx.sources->get<TcpSource>(NonDeviceSource::TCP_SOURCE_0)](const std::shared_ptr<TcpConnection>& connection) {
        if (tcp_source != nullptr) {
            tcp_source->registerConnection(connection);
        }else{
            logError("Nullpointer in creating connection handler for tcpsource. Make sure sources are initialized beforehand.");
        }
    });

    // --- maintenance ---
    ctx.scheduler->every(std::chrono::seconds(5), [server = ctx.server.get()]() {
        server->heartbeatAndCleanup(std::chrono::seconds(30));
    });

    return ctx;
}
