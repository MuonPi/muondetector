#ifndef MQTTLOGSINK_H
#define MQTTLOGSINK_H

#include "abstractsink.h"
#include "mqttlink.h"
#include "log.h"
#include "clusterlog.h"
#include "detectorlog.h"

#include <memory>
#include <string>
#include <ctime>
#include <iomanip>

namespace MuonPi {


template <typename T>
/**
 * @brief The MqttLogSink class
 */
class MqttLogSink : public AbstractSink<T>
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

        template<typename U>
        auto operator<<(U value) -> Constructor& {
            m_stream<<' '<<value;
            return *this;
        }

        auto str() -> std::string
        {
            return m_stream.str();
        }

    private:
        std::ostringstream m_stream;
    };

    void process(T log);

    void fix_time();
    [[nodiscard]] auto construct(const std::string& parname) -> Constructor;
    [[nodiscard]] auto publish(Constructor &constructor) -> bool;

    std::chrono::system_clock::time_point m_time {};
    MqttLink::Publisher m_link;
};


template <typename T>
MqttLogSink<T>::MqttLogSink(MqttLink::Publisher& publisher)
    : AbstractSink<ClusterLog>{}
    , m_link { std::move(publisher) }
{
    AbstractSink<T>::start();
}

template <typename T>
MqttLogSink<T>::~MqttLogSink() = default;

template <typename T>
auto MqttLogSink<T>::step() -> int
{
    if (AbstractSink<T>::has_items()) {
        process(AbstractSink<T>::next_item());
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

template <typename T>
void MqttLogSink<T>::fix_time()
{
    m_time = std::chrono::system_clock::now();
}


template <typename T>
auto MqttLogSink<T>::construct(const std::string& parname) -> Constructor
{
    std::ostringstream stream{};

    std::time_t time { std::chrono::system_clock::to_time_t(m_time) };

    stream<<std::put_time(std::gmtime(&time), "%F_%H-%M-%S")<<' '<<parname;

    return Constructor{ std::move(stream) };
}

template <>
void MqttLogSink<ClusterLog>::process(ClusterLog log)
{
    fix_time();
    if (!(
                m_link.publish((construct("timeout")<<log.data().timeout).str())
                && m_link.publish((construct("frequency_in")<<log.data().frequency.single_in).str())
                && m_link.publish((construct("frequency_l1_out")<<log.data().frequency.l1_out).str())
                && m_link.publish((construct("buffer_length")<<log.data().buffer_length).str())
                && m_link.publish((construct("total_detectors")<<log.data().total_detectors).str())
                && m_link.publish((construct("reliable_detectors")<<log.data().reliable_detectors).str())
                && m_link.publish((construct("max_coincidences")<<log.data().maximum_n).str())
                && m_link.publish((construct("incoming")<<log.data().incoming).str())
          )) {
        Log::warning()<<"Could not publish MQTT message.";
        return;
    }
    for (auto& [level, n]: log.data().outgoing) {
        if (level == 1) {
            continue;
        }
        if (!m_link.publish((construct("outgoing_" + std::to_string(level))<<n).str())) {
            Log::warning()<<"Could not publish MQTT message.";
            return;
        }

    }
}

template <>
void MqttLogSink<DetectorLog>::process(DetectorLog log)
{
    fix_time();
    std::string name { log.user_info().username + " " + log.user_info().station_id};
    if (!(
                m_link.publish((construct(name + " eventrate")<<log.data().mean_eventrate).str())
                && m_link.publish((construct(name + " pulselength")<<log.data().mean_pulselength).str())
                && m_link.publish((construct(name + " incoming")<<log.data().incoming).str())
                && m_link.publish((construct(name + " ublox_counter_progess")<<log.data().ublox_counter_progress).str())
                && m_link.publish((construct(name + " deadtime_factor")<<log.data().deadtime).str())
          )) {
        Log::warning()<<"Could not publish MQTT message.";
    }
}

}

#endif // MQTTLOGSINK_H
