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
    std::vector<std::string> message_strings;
	
	message_strings.push_back(construct_message("timeout",std::to_string(log.data().timeout)));
	message_strings.push_back(construct_message("frequency_in",std::to_string(log.data().frequency.single_in)));
	message_strings.push_back(construct_message("frequency_l1_out",std::to_string(log.data().frequency.l1_out)));
	message_strings.push_back(construct_message("buffer_length",std::to_string(log.data().buffer_length)));
	message_strings.push_back(construct_message("total_detectors",std::to_string(log.data().total_detectors)));
	message_strings.push_back(construct_message("reliable_detectors",std::to_string(log.data().reliable_detectors)));
	message_strings.push_back(construct_message("max_coincidences",std::to_string(log.data().maximum_n)));
	message_strings.push_back(construct_message("frequency_in",std::to_string(log.data().incoming)));
	message_strings.push_back(construct_message("frequency_out_n2",std::to_string(log.data().outgoing[2])));
	message_strings.push_back(construct_message("frequency_out_n3",std::to_string(log.data().outgoing[3])));
	message_strings.push_back(construct_message("frequency_out_n4",std::to_string(log.data().outgoing[4])));
	message_strings.push_back(construct_message("frequency_out_n5",std::to_string(log.data().outgoing[5])));
	

	
	for (auto& str: message_strings) {
		if (m_link.publish(str)) {
			Log::warning()<<"Could not publish MQTT message.";
		}
	}
}

auto MqttLogSink::construct_message(const std::string& parname, const std::string& value_string) -> std::string
{
	// TODO: Put current date and time in string repreentation
	const std::string datestr = " ";
	std::string str { datestr };
	str += " ";
	str += parname;
	str += " ";
	str += value_string;
	return str;
}

}
