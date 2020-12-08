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
    std::size_t i { 0 };
    while (has_items() && (i < 5)) {
        std::unique_ptr<Event> e { next_item()};
        if (e->n() > 1) {
            std::unique_ptr<CombinedEvent> evt_ptr {dynamic_cast<CombinedEvent*>(e.get())};
            process(std::move(evt_ptr));
        } else {
            process(std::move(e));
        }
        i++;
    }
    return {};
}

void MqttEventSink::process(std::unique_ptr<Event> /*evt*/)
{
}

void MqttEventSink::process(std::unique_ptr<CombinedEvent> /*evt*/)
{
}

}
