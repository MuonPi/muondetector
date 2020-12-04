#include "mqtteventsink.h"

#include "event.h"

namespace MuonPi {

MqttEventSink::MqttEventSink(std::shared_ptr<MqttLink::Publisher> publisher)
    : m_link { publisher }
{

}

MqttEventSink::~MqttEventSink()
{
    AbstractSink<Event>::~AbstractSink();
}

auto MqttEventSink::step() -> int
{
    return {};
}

}
