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
    struct Publishers {
        MqttLink::Publisher& single;
        MqttLink::Publisher& combined;
    };
    /**
     * @brief MqttEventSink
     * @param publishers The topics from which the messages should be published
     */
    MqttEventSink(Publishers publishers);

    ~MqttEventSink() override;
protected:
    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:

    void process(Event evt);

    Publishers m_link;
};

}

#endif // MQTTEVENTSINK_H
