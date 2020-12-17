#include "mqtteventsource.h"

#include "event.h"
#include "utility.h"
#include "log.h"

#include <cmath>

namespace MuonPi {

MqttEventSource::MqttEventSource(Subscribers subscribers)
    : AbstractSource<Event>{}
    , m_link { std::move(subscribers) }
{
    start();
}

MqttEventSource::~MqttEventSource() = default;


auto MqttEventSource::step() -> int
{
    if (m_link.single.has_message()) {
        MqttLink::Message msg = m_link.single.get_message();
        MessageParser topic { msg.topic, '/'};
        MessageParser content { msg.content, ' '};
        if ((topic.size() >= 4) && (content.size() >= 7)) {
            std::string name {topic[3] };
            for (std::size_t i = 4; i < topic.size(); i++) {
                name += "/" + topic[i];
            }
            std::size_t hash { std::hash<std::string>{}(topic[2] + name) };

            Event::Data data;
            try {
                MessageParser start {content[0], '.'};
                if (start.size() != 2) {
                    Log::warning()<<"Message '" + msg.topic + " " + msg.content + "' is invalid.";
                    return 0;
                }
                data.epoch = std::stoll(start[0]);
                data.start = std::stoll(start[1]) * static_cast<std::int_fast64_t>(std::pow(10, (9 - start[1].length())));

            } catch (...) {
                Log::warning()<<"Message '" + msg.topic + " " + msg.content + "' is invalid.";
                return 0;
            }


            try {
                MessageParser start {content[1], '.'};
                if (start.size() != 2) {
                    Log::warning()<<"Message '" + msg.topic + " " + msg.content + "' is invalid.";
                    return 0;
                }
                std::int_fast64_t epoch { std::stoll(start[0]) };
                std::int_fast64_t end { std::stoll(start[1]) * static_cast<std::int_fast64_t>(std::pow(10, (9 - start[1].length()))) };

                std::int_fast64_t d_epoch {epoch - data.epoch};
                std::int_fast64_t d_offset {end - data.start};

                data.end = d_epoch * static_cast<std::int_fast64_t>(1e9) + d_offset;

            } catch (...) {
                Log::warning()<<"Message '" + msg.topic + " " + msg.content + "' is invalid.";
                return 0;
            }

            std::uint64_t id {hash & 0xFFFFFFFF00000000 + 0x00000000FFFFFFFF & static_cast<std::uint64_t>(data.epoch)};

            try {
                data.user = topic[2];
                data.station_id = name;
                data.time_acc = static_cast<std::uint32_t>(std::stoul(content[2], nullptr));
                data.ublox_counter = static_cast<std::uint16_t>(std::stoul(content[3], nullptr));
                data.fix = static_cast<std::uint8_t>(std::stoul(content[4], nullptr));
                data.utc = static_cast<std::uint8_t>(std::stoul(content[5], nullptr));
                data.gnss_time_grid = static_cast<std::uint8_t>(std::stoul(content[6], nullptr));
            } catch (std::invalid_argument& e) {
                Log::warning()<<"Received exception: " + std::string(e.what()) + "\n While converting '" + msg.topic + " " + msg.content + "'";
                return 0;
            }
            push_item(Event{hash,id,data});
        }
    }
    if (m_link.combined.has_message()) {
        MqttLink::Message msg = m_link.combined.get_message();
        MessageParser topic { msg.topic, '/'};
        MessageParser content { msg.content, ' '};
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

}
