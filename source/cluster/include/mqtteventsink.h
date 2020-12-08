#ifndef MQTTEVENTSINK_H
#define MQTTEVENTSINK_H

#include "abstractsink.h"
#include "mqttlink.h"

#include <memory>

namespace MuonPi {

class Event;
class CombinedEvent;

/**
 * @brief The MqttEventSink class
 */
class MqttEventSink : public AbstractSink<Event>
{
public:
    struct Publishers {
        std::shared_ptr<MqttLink::Publisher> single { nullptr };
        std::shared_ptr<MqttLink::Publisher> combined { nullptr };
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

    void process(std::unique_ptr<Event> evt);
    void process(std::unique_ptr<CombinedEvent> evt);

    Publishers m_link {};
};

}

#endif // MQTTEVENTSINK_H
