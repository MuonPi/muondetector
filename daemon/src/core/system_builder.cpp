#include "core/system_builder.h"
#include "app/system_config.h"
#include "core/thread_pool.h"
#include "core/scheduler.h"

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

// Data
#include "data/ad1115_event.h"

#include <memory>


SystemBuilder::Context SystemBuilder::build(ThreadPool& pool, const SystemConfig& config, Scheduler& scheduler)
{
    Context ctx;

    ctx.io = std::make_shared<boost::asio::io_context>();
    ctx.registry = std::make_unique<DeviceRegistry>();
    ctx.sources = std::make_unique<SourceManager>();
    ctx.sinks = std::make_unique<SinkManager>();
    ctx.bus = std::make_unique<EventBus>(pool);

    // --- hardware ---
    DeviceFactory::createADS1115(ctx.registry, 1, "/dev/i2c-1", 0x48);
    DeviceFactory::createADS1115(ctx.registry, 2, "/dev/i2c-1", 0x49);

    // --- sources ---
    SourceFactory::createADS1115Source(ctx.sources, 1, ctx.registry, ctx.bus);
    SourceFactory::createADS1115Source(ctx.sources, 2, ctx.registry, ctx.bus);

    // --- sinks ---
    auto tcp_sink = SinkFactory::createTcpSink(ctx.sinks);


    ctx.bus->subscribe<Ad1115SampleEvent>(std::bind(&TcpSink::handle, tcp_sink, std::placeholders::_1));

    // --- tcp_server ---
    ctx.server = std::make_unique<TcpServer>(ctx.io, config.serverPort, tcp_sink);

    return ctx;
}
