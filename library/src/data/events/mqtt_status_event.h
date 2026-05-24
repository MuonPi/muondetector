#ifndef MQTT_STATUS_EVENT_H
#define MQTT_STATUS_EVENT_H

#include <string>

struct MqttStatusEvent {
    enum class Status {
        Invalid,
        Connected,
        Disconnected,
        Connecting,
        Disconnecting,
        Error,
        Inhibited
    };
    Status status{Status::Invalid};
    std::string text{};
};

#endif // MQTT_STATUS_EVENT_H