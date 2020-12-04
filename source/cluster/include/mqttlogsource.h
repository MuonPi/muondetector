#ifndef MQTTLOGSOURCE_H
#define MQTTLOGSOURCE_H

#include "abstractsource.h"
#include "mqttlink.h"

#include <memory>

namespace MuonPi {

class LogMessage;

/**
 * @brief The MqttLogSource class
 */
class MqttLogSource : public AbstractSource<LogMessage>
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

#endif // MQTTLOGSOURCE_H
