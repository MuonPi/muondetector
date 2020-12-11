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
    const std::string DATA_TOPIC_STR {"muonpi/data"};
    if (m_link.single->has_message()) {
        MqttLink::Message msg = m_link.single->get_message();
        MessageParser topic { msg.topic, '/'};
        MessageParser content { msg.content, ' '};
        if ((topic.size() == 4) && (content.size() >= 2)) {

            std::size_t hash {std::hash<std::string>{}(topic[2] + topic[3])};

            std::string ts_string = content[0];
            std::uint64_t ts = std::stoull(ts_string, nullptr);

            std::chrono::steady_clock::time_point start = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(ts));

            ts_string = content[1];
            ts = std::stoull(ts_string, nullptr);

            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(ts));

            std::uint64_t id {hash & 0xFFFFFFFF00000000 + 0x00000000FFFFFFFF & ts};

            push_item(std::make_unique<Event>(Event(hash,id,start,end)));
        }
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
