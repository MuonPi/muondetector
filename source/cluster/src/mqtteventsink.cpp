#include "mqtteventsink.h"

#include "event.h"

namespace MuonPi {

MqttEventSink::MqttEventSink(std::shared_ptr<MqttLink::Publisher> publisher)
    : m_link { publisher }
{

}

MqttEventSink::~MqttEventSink() = default;

auto MqttEventSink::step() -> int
{
    return {};
}

}
