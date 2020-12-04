#ifndef MQTTEVENTSOURCE_H
#define MQTTEVENTSOURCE_H

#include "abstractsource.h"

#include <memory>

namespace MuonPi {

class Event;
class MqttLink;

/**
 * @brief The MqttEventSource class
 */
class MqttEventSource : public AbstractSource<Event>
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

#endif // MQTTEVENTSOURCE_H
