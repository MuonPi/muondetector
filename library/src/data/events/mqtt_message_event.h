#ifndef MQTT_MESSAGE_EVENT_H
#define MQTT_MESSAGE_EVENT_H

#include <string>

struct MqttMessageEvent {
    enum DIRECTION { IN, OUT };
    DIRECTION direction{OUT};
    std::string msg{};
};

#endif // MQTT_MESSAGE_EVENT_H