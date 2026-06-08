#ifndef SINK_FACTORY_H
#define SINK_FACTORY_H

#include "core/event_bus.h"
#include "core/registries/sink_manager.h"
#include "sinks/file_sink.h"
#include "sinks/mqtt_sink.h"
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

    static auto createMqttSink(EventBus& bus, std::unique_ptr<SinkManager>& sinks,
                               const std::string& station_id) -> std::shared_ptr<MqttSink> {
        auto mqtt_sink = std::make_shared<MqttSink>(bus, station_id);
        sinks->add(mqtt_sink);
        return mqtt_sink;
    }

    static auto createFileSink(EventBus& bus,
                               std::unique_ptr<SinkManager>& sinks) -> std::shared_ptr<FileSink> {
        auto file_sink = std::make_shared<FileSink>(bus);
        sinks->add(file_sink);
        return file_sink;
    }
};

#endif // SINK_FACTORY_H