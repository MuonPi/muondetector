#include "mqttlogsource.h"

#include "logmessage.h"

namespace MuonPi {

MqttLogSource::MqttLogSource(std::shared_ptr<MqttLink::Subscriber> subscriber)
    : m_link { subscriber }
{
}

MqttLogSource::~MqttLogSource()
{
    AbstractSource<LogMessage>::~AbstractSource();
}

auto MqttLogSource::step() -> int
{
    return {};
}

}
