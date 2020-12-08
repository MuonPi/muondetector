#include "mqttlogsource.h"

#include "logmessage.h"

namespace MuonPi {

MqttLogSource::MqttLogSource(std::shared_ptr<MqttLink::Subscriber> subscriber)
    : m_link { subscriber }
{
}

MqttLogSource::~MqttLogSource() = default;

auto MqttLogSource::step() -> int
{
    if (m_link->has_message()) {
        std::string msg = m_link->get_message();
        // todo: parsing of message
        push_item(nullptr);
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

}
