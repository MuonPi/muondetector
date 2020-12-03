#ifndef MQTTEVENTSINK_H
#define MQTTEVENTSINK_H

#include "abstractsink.h"

#include <memory>

namespace MuonPi {

class Event;
class MqttLink;

class MqttEventSink : public AbstractSink<Event>
{
protected:
    /**
     * @brief step implementation from ThreadRunner. In case of a false return value, the event loop will stop.
     * @return true if the step succeeded.
     */
    [[nodiscard]] auto step() -> bool override;

private:
    std::unique_ptr<MqttLink> m_link { nullptr };
};

}

#endif // MQTTEVENTSINK_H
