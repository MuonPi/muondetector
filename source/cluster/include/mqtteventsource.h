#ifndef MQTTEVENTSOURCE_H
#define MQTTEVENTSOURCE_H

#include "abstractsource.h"
#include "mqttlink.h"

#include <memory>

namespace MuonPi {

class Event;

/**
 * @brief The MqttEventSource class
 */
class MqttEventSource : public AbstractSource<Event>
{
public:
    struct Subscribers {
        MqttLink::Subscriber& single;
        MqttLink::Subscriber& combined;
    };
    /**
     * @brief MqttEventSource
     * @param subscriber The Mqtt Topic this event source should be subscribed to
     */
    MqttEventSource(Subscribers subscribers);

    ~MqttEventSource() override;
protected:
    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;
    [[nodiscard]] auto pre_run() -> int override;

private:
    Subscribers m_link;
};

}

#endif // MQTTEVENTSOURCE_H
