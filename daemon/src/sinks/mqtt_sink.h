#ifndef MQTT_SINK_H
#define MQTT_SINK_H

#include "data/events/mqtt_message_event.h"
#include "sinks/sink.h"

class MqttSink : public Sink {
  public:
    MqttSink() = default;
    ~MqttSink() = default;
    void handle(const MqttMessageEvent& event);
};
#endif // MQTT_SINK_H