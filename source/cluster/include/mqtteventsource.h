#ifndef MQTTEVENTSOURCE_H
#define MQTTEVENTSOURCE_H

#include "abstractsource.h"

#include <memory>

namespace MuonPi {

class Event;
class MqttLink;

class MqttEventSource : public AbstractSource<Event>
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

#endif // MQTTEVENTSOURCE_H
