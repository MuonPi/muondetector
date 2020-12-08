#include "mqtteventsink.h"

#include "event.h"
#include "combinedevent.h"

namespace MuonPi {

MqttEventSink::MqttEventSink(std::shared_ptr<MqttLink::Publisher> publisher)
    : m_link { publisher }
{

}

MqttEventSink::~MqttEventSink() = default;

auto MqttEventSink::step() -> int
{
    std::size_t i { 0 };
    while (has_items() && (i < 5)) {
        std::unique_ptr<Event> e { next_item()};
        if (e->n() > 1) {
            process(std::make_unique<CombinedEvent>(dynamic_cast<CombinedEvent*>(e.get())));
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
