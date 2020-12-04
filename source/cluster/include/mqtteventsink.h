#ifndef MQTTEVENTSINK_H
#define MQTTEVENTSINK_H

#include "abstractsink.h"
#include "mqttlink.h"

#include <memory>

namespace MuonPi {

class Event;

/**
 * @brief The MqttEventSink class
 */
class MqttEventSink : public AbstractSink<Event>
{
public:
    /**
     * @brief MqttEventSink
     * @param publisher The topic from which the messages should be published
     */
    MqttEventSink(std::shared_ptr<MqttLink::Publisher> publisher);
protected:
    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:
    std::shared_ptr<MqttLink::Publisher> m_link { nullptr };
};

}

#endif // MQTTEVENTSINK_H
