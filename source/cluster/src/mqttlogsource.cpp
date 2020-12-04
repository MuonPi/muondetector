#include "mqttlogsource.h"

#include "logmessage.h"

namespace MuonPi {

MqttLogSource::MqttLogSource(std::shared_ptr<MqttLink::Subscriber> subscriber)
    : m_link { subscriber }
{
}

auto MqttLogSource::step() -> int
{
    return {};
}

}
