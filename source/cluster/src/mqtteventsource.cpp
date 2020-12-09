#include "mqtteventsource.h"

#include "event.h"
#include "utility.h"

namespace MuonPi {

MqttEventSource::MqttEventSource(Subscribers subscribers)
    : m_link { std::move(subscribers) }
{

}

MqttEventSource::~MqttEventSource() = default;


auto MqttEventSource::step() -> int
{
    if (m_link.single->has_message()) {
        MqttLink::Message msg = m_link.single->get_message();
        MessageParser topic { msg.topic, '/'};
        MessageParser content { msg.content, ' '};
        std::unique_ptr<Event> event { nullptr };
        // todo: parsing of message
        push_item(std::move(event));
    }
    if (m_link.combined->has_message()) {
        MqttLink::Message msg = m_link.combined->get_message();
        MessageParser topic { msg.topic, '/'};
        MessageParser content { msg.content, ' '};
        std::unique_ptr<Event> event { nullptr };
        // todo: parsing of message
        push_item(std::move(event));
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

}
