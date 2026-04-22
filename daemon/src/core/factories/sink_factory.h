#ifndef SINK_FACTORY_H
#define SINK_FACTORY_H

#include "core/registries/sink_manager.h"
#include "sinks/sink.h"
#include "sinks/tcp_sink.h"

#include <memory>

class SinkFactory {
  public:
    static auto createTcpSink(std::unique_ptr<SinkManager>& sinks) -> std::shared_ptr<TcpSink> {
        auto tcp_sink = std::make_shared<TcpSink>();
        sinks->add(tcp_sink);
        return tcp_sink;
    }
};

#endif // SINK_FACTORY_H