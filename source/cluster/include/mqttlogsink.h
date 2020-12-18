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
    struct Publishers {
        MqttLink::Publisher& single;
        MqttLink::Publisher& combined;
    };
    /**
     * @brief MqttLogSink
     * @param publishers The topics from which the messages should be published
     */
    MqttLogSink(Publishers publishers);

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

    Publishers m_link;
};

}

#endif // MQTTLOGSINK_H
