#include "mqtteventsource.h"

#include "event.h"

namespace MuonPi {

MqttEventSource::MqttEventSource(Subscribers subscribers)
    : m_link { std::move(subscribers) }
{

}

MqttEventSource::~MqttEventSource() = default;

auto MqttEventSource::step() -> int
{
    return {};
}

}
