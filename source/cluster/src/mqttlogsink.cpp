#include "mqttlogsink.h"
#include "clusterlog.h"
#include "log.h"
#include "utility.h"


namespace MuonPi {

MqttLogSink::MqttLogSink(MqttLink::Publisher& publisher)
    : AbstractSink<ClusterLog>{}
    , m_link { std::move(publisher) }
{
    start();
}

MqttLogSink::~MqttLogSink() = default;

auto MqttLogSink::step() -> int
{
    if (has_items()) {
        process(next_item());
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

void MqttLogSink::process(ClusterLog log)
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

void MqttLogSink::fix_time()
{
    m_time = std::chrono::system_clock::now();
}


auto MqttLogSink::construct(const std::string& parname) -> Constructor
{
    std::ostringstream stream{};

    std::time_t time { std::chrono::system_clock::to_time_t(m_time) };

    stream<<std::put_time(std::gmtime(&time), "%F_%H-%M-%S")<<' '<<parname;

    return Constructor{ std::move(stream) };
}
}
