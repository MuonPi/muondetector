#ifndef MQTTLOGSINK_H
#define MQTTLOGSINK_H

#include "abstractsink.h"
#include "mqttlink.h"

#include <memory>
#include <string>

namespace MuonPi {

class ClusterLog;

/**
 * @brief The MqttLogSink class
 */
class MqttLogSink : public AbstractSink<ClusterLog>
{
public:
    /**
     * @brief MqttLogSink
     * @param publishers The topics from which the messages should be published
     */
    MqttLogSink(MqttLink::Publisher& publisher);

    ~MqttLogSink() override;
	
protected:
    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:

    void process(ClusterLog log);
	auto construct_message(const std::string& parname, const std::string& value_string) -> std::string;

    MqttLink::Publisher m_link;
};

}

#endif // MQTTLOGSINK_H
