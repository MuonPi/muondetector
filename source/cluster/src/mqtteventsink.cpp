#include "mqtteventsink.h"
#include "event.h"
#include "combinedevent.h"
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
        std::unique_ptr<Event> e { next_item()};
        if (e->n() > 1) {
            std::unique_ptr<CombinedEvent> evt_ptr {dynamic_cast<CombinedEvent*>(e.get())};
            process(std::move(evt_ptr));
        } else {
            process(std::move(e));
        }
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

void MqttEventSink::process(std::unique_ptr<Event> /*evt*/)
{
    MessageConstructor message {' '};

    // todo: construct message string

    if (m_link.single->publish(message.get_string())) {
        syslog(Log::Warning, "Could not publish MQTT message.");
    }
}

void MqttEventSink::process(std::unique_ptr<CombinedEvent> /*evt*/)
{
    MessageConstructor message {' '};

    // todo: construct message string

    if (m_link.combined->publish(message.get_string())) {
        syslog(Log::Warning, "Could not publish MQTT message.");
    }
}

}
