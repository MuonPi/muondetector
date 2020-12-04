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
public:
    /**
     * @brief MqttLogSource
     * @param subscriber The Mqtt Topic this Log source should be subscribed to
     */
    MqttLogSource(std::shared_ptr<MqttLink::Subscriber> subscriber);

    ~MqttLogSource() override;

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

#endif // MQTTLOGSOURCE_H
