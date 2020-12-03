#ifndef MQTTLOGSOURCE_H
#define MQTTLOGSOURCE_H

#include "abstractsource.h"

#include <memory>

namespace MuonPi {

class LogMessage;
class MqttLink;

/**
 * @brief The MqttLogSource class
 */
class MqttLogSource : public AbstractSource<LogMessage>
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

#endif // MQTTLOGSOURCE_H
