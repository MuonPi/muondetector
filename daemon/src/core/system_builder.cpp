#include "core/system_builder.h"
#include "app/system_config.h"
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


static void loadHardwareConfig(const std::string& path)
{
}

static void loadSourcesConfig(const std::string& path)
{

}

SystemBuilder::Context SystemBuilder::build(ThreadPool& pool, const SystemConfig& config)
{
    Context ctx;

    ctx.io = std::make_shared<boost::asio::io_context>();
    ctx.registry = std::make_unique<DeviceRegistry>();
    ctx.sources = std::make_unique<SourceManager>();
    ctx.sinks = std::make_unique<SinkManager>();
    ctx.bus = std::make_unique<EventBus>(pool);
    ctx.scheduler = std::make_unique<Scheduler>(pool);

    // --- hardware ---
    ctx.registry->add(Device::AD1115, DeviceFactory::createADS1115("/dev/i2c-1", 0x48));

    // --- sources ---
    auto ads1115Source = SourceFactory::createADS1115Source(Device::AD1115, ctx.registry, ctx.bus);
    auto tcp_source = SourceFactory::createTcpSource(ctx.sources, ctx.bus);

    ctx.sources->add(ads1115Source);
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

    // --- read sensors ---
    ctx.scheduler->every(std::chrono::seconds(1), std::bind(&ADS1115Source::update, ads1115Source));

    ctx.scheduler->every(std::chrono::seconds(1), [source = ads1115Source]() {
        source->update();
    });

    return ctx;
}
