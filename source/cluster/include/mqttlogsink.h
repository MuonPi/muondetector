#ifndef MQTTLOGSINK_H
#define MQTTLOGSINK_H

#include "abstractsink.h"
#include "mqttlink.h"
#include "log.h"

#include <memory>
#include <string>
#include <ctime>
#include <iomanip>

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
     * @param publisher The topic from which the messages should be published
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

    class Constructor
    {
    public:
        Constructor(std::ostringstream stream)
            : m_stream { std::move(stream) }
        {}

        template<typename T>
        auto operator<<(T value) -> Constructor& {
            m_stream<<value;
            return *this;
        }

        auto str() -> std::string
        {
            return m_stream.str();
        }

    private:
        std::ostringstream m_stream;
    };

    void process(ClusterLog log);

    void fix_time();
    [[nodiscard]] auto construct(const std::string& parname) -> Constructor;
    [[nodiscard]] auto publish(Constructor &constructor) -> bool;

    std::chrono::system_clock::time_point m_time {};
    MqttLink::Publisher m_link;
};

}

#endif // MQTTLOGSINK_H
