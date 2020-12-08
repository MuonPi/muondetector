#include "mqtteventsink.h"

#include "event.h"
#include "combinedevent.h"

namespace MuonPi {

MqttEventSink::MqttEventSink(Publishers publishers)
    : m_link { std::move(publishers) }
{

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
    std::string message {};

    // todo: construct message string

    if (m_link.single->publish(message)) {
        // todo: error handling
    }
}

void MqttEventSink::process(std::unique_ptr<CombinedEvent> /*evt*/)
{
    std::string message {};

    // todo: construct message string

    if (m_link.combined->publish(message)) {
        // todo: error handling
    }
}

}
