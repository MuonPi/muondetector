#ifndef CONTEXT_H
#define CONTEXT_H


#include "core/registries/device_registry.h"
#include "core/registries/component_manager.h"
#include "core/registries/sink_manager.h"
#include "core/event_bus.h"

#include "app/system_config.h"
#include "core/thread_pool.h"
#include "core/scheduler.h"
#include "network/tcpserver.h"

#include <boost/asio.hpp>
#include <memory>


struct Context
{
    std::shared_ptr<boost::asio::io_context> io;
    std::unique_ptr<DeviceRegistry> registry;
    std::unique_ptr<ComponentManager> components;
    std::unique_ptr<SinkManager> sinks;
    std::unique_ptr<EventBus> bus;
    std::unique_ptr<TcpServer> server;
    std::unique_ptr<Scheduler> scheduler;
    std::unique_ptr<SystemConfig> config;
};

#endif // CONTEXT_H