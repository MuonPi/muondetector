#ifndef MQTT_LOG_EVENT_H
#define MQTT_LOG_EVENT_H

#include <string>

struct MqttLogEvent {
    bool log{false};
    std::string msg{};
};

#endif // MQTT_LOG_EVENT_H