#ifndef SYSTEM_BUILDER_H
#define SYSTEM_BUILDER_H

#include "app/system_config.h"
#include "core/thread_pool.h"
#include "core/scheduler.h"

#include "core/registries/device_registry.h"
#include "core/registries/source_manager.h"
#include "core/registries/sink_manager.h"
#include "core/event_bus.h"
#include "network/tcpserver.h"

#include <boost/asio.hpp>
#include <memory>

struct DeviceConfig;
struct SourceConfig;
class SystemBuilder
{
public:
    struct Context
    {
        std::shared_ptr<boost::asio::io_context> io;
        std::unique_ptr<DeviceRegistry> registry;
        std::unique_ptr<SourceManager> sources;
        std::unique_ptr<SinkManager> sinks;
        std::unique_ptr<EventBus> bus;
        std::unique_ptr<TcpServer> server;
        std::unique_ptr<Scheduler> scheduler;
    };

    static auto parseHardwareConfig(Context& ctx, const libconfig::Config& hardwareConfig) -> std::vector<DeviceConfig>;
    static auto parseSourcesConfig(Context& ctx, const libconfig::Config& sourcesConfig) -> std::vector<SourceConfig>;
    static Context build(ThreadPool& pool, const SystemConfig& config);
};

#endif // SYSTEM_BUILDER_H