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
    /**
     * @brief MqttEventSource
     * @param subscriber The Mqtt Topic this event source should be subscribed to
     */
    MqttEventSource(std::shared_ptr<MqttLink::Subscriber> subscriber);

protected:
    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:
    std::shared_ptr<MqttLink::Subscriber> m_link { nullptr };
};

}

#endif // MQTTEVENTSOURCE_H
