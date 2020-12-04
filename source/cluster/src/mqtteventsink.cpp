#include "mqtteventsink.h"

#include "event.h"

namespace MuonPi {

MqttEventSink::MqttEventSink(std::shared_ptr<MqttLink::Publisher> publisher)
    : m_link { publisher }
{

}
auto MqttEventSink::step() -> int
{
    return {};
}

}
