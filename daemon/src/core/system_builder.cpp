#include "core/system_builder.h"
#include "app/system_config.h"
#include "app/config_parser.h"
#include "core/thread_pool.h"
#include "core/scheduler.h"
#include "core/logging/logger.h"

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

// Data
#include "data/ad1115_event.h"

#include <memory>
#include <chrono>

std::vector<DeviceConfig> parseDevices(const libconfig::Setting& root)
{
}

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

            cfg.device = dev;
            if (cfg.address < 0 || cfg.address > std::numeric_limits<std::uint8_t>::max()) {
                logError("Hardware address parsing error, outside uint8_t range. Ignoring device " + idStr);
                continue;
            }
            cfg.address = static_cast<std::uint8_t>(address);
        }
        else if (cfg.category == "uart")
        {
            std::string dev;
            int baud;

            d.lookupValue("device", dev);
            d.lookupValue("baud", baud);

            cfg.device = dev;
            cfg.baud = baud;
        }

        result.push_back(cfg);
    }
}

void SystemBuilder::parseSourcesConfig(Context& ctx, const libconfig::Config& sourcesConfig)
{

}

SystemBuilder::Context SystemBuilder::build(ThreadPool& pool, const SystemConfig& config)
{
    // First try loading libconfig data
    auto hardwareConfig = ConfigParser::loadConfigFile(config.hardwareConfigPath);
    auto sourcesConfig = ConfigParser::loadConfigFile(config.sourcesConfigPath);

    // Build System
    Context ctx;

    ctx.io = std::make_shared<boost::asio::io_context>();
    ctx.registry = std::make_unique<DeviceRegistry>();
    ctx.sources = std::make_unique<SourceManager>();
    ctx.sinks = std::make_unique<SinkManager>();
    ctx.bus = std::make_unique<EventBus>(pool);
    ctx.scheduler = std::make_unique<Scheduler>(pool);

    std::vector<DeviceConfig> deviceConfigurations = SystemBuilder::parseHardwareConfig(ctx, *hardwareConfig);


    // --- hardware ---
    for (auto& config : deviceConfigurations) {
        ctx.registry->add(config.id, DeviceFactory::create(config));
        auto source = SourceFactory::createDeviceSource(config.id, *ctx.registry, *ctx.bus);
        ctx.scheduler->every(std::chrono::seconds(5), [source]() {
            source->update();
        });
    }

    // --- sources ---
    auto tcp_source = SourceFactory::createTcpSource(*ctx.bus);

    ctx.sources->add(tcp_source);

    // --- sinks ---
    auto tcp_sink = SinkFactory::createTcpSink(ctx.sinks);


    // make tcp sink send data through tcp connections
    ctx.bus->subscribe<Ads1115Event>(std::bind(&TcpSink::handle, tcp_sink, std::placeholders::_1));

    // --- tcp_server ---
    // When server accepts a new TCP connection, call this handler.
    ctx.server = std::make_unique<TcpServer>(ctx.io, config.serverPort, tcp_sink);
    ctx.server->addConnectionHandler([tcp_source](const std::shared_ptr<TcpConnection>& connection) {
        tcp_source->registerConnection(connection);
    });

    // --- maintenance ---
    ctx.scheduler->every(std::chrono::seconds(5), [server = ctx.server.get()]() {
        server->heartbeatAndCleanup(std::chrono::seconds(30));
    });

    return ctx;
}
