#include "mqtteventsink.h"
#include "event.h"
#include "log.h"
#include "utility.h"

namespace MuonPi {

MqttEventSink::MqttEventSink(Publishers publishers)
    : AbstractSink<Event>{}
    , m_link { std::move(publishers) }
{
    start();
}

MqttEventSink::~MqttEventSink() = default;

auto MqttEventSink::step() -> int
{
    if (has_items()) {
        process(next_item());
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

void MqttEventSink::process(Event /*evt*/)
{
    MessageConstructor message {' '};

    // todo: construct message string

    if (m_link.single.publish(message.get_string())) {
        Log::warning()<<"Could not publish MQTT message.";
    }
}

}
