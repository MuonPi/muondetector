#ifndef MQTTEVENTSINK_H
#define MQTTEVENTSINK_H

#include "abstractsink.h"

#include <memory>

namespace MuonPi {

class Event;
class MqttLink;

/**
 * @brief The MqttEventSink class
 */
class MqttEventSink : public AbstractSink<Event>
{
protected:
    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:
    std::unique_ptr<MqttLink> m_link { nullptr };
};

}

#endif // MQTTEVENTSINK_H
