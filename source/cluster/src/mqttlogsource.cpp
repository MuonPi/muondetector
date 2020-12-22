#include "mqttlogsource.h"

#include "utility.h"
#include "log.h"

namespace MuonPi {

void DetectorInfoCollector::reset()
{
    user_info = UserInfo { };
    status = s_default_status;
}

auto DetectorInfoCollector::add(MessageParser& message) -> bool
{
    if (message_id != message[0]) {
        reset();
        message_id = message[0];
    }
    try {
        if (message[1] == "geoHeightMSL") {
            geo.h = std::stod(message[2], nullptr);
            status &= ~1;
        } else if (message[1] == "geoHorAccuracy") {
            geo.h_acc = std::stod(message[2], nullptr);
            status &= ~2;
        } else if (message[1] == "geoLatitude") {
            geo.lat = std::stod(message[2], nullptr);
            status &= ~4;
        } else if (message[1] == "geoLongitude") {
            geo.lon = std::stod(message[2], nullptr);
            status &= ~8;
        } else if (message[1] == "geoVertAccuracy") {
            geo.v_acc = std::stod(message[2], nullptr);
            status &= ~16;
        } else if (message[1] == "positionDOP") {
            geo.dop = std::stod(message[2], nullptr);
            status &= ~32;
        } else if (message[1] == "timeAccuracy") {
            time.accuracy = std::stod(message[2], nullptr);
            status &= ~64;
        } else if (message[1] == "timeDOP") {
            time.dop = std::stod(message[2], nullptr);
            status &= ~128;
        } else {
            return false;
        }
    } catch(std::invalid_argument& e) {
        Log::warning()<<"received exception when parsing log item: " + std::string(e.what());
        return false;
    }

    return true;
}

auto DetectorInfoCollector::complete() -> bool
{
    return !status;
}

MqttLogSource::MqttLogSource(MqttLink::Subscriber& subscriber)
    : m_link { subscriber }
{
    start();
}

MqttLogSource::~MqttLogSource() = default;

auto MqttLogSource::pre_run() -> int
{
    return 0;
}

auto MqttLogSource::step() -> int
{
    if (m_link.has_message()) {
        MqttLink::Message msg = m_link.get_message();
        MessageParser topic { msg.topic, '/'};
        MessageParser content { msg.content, ' '};
        if ((topic.size() >= 4) && (content.size() >= 2)) {
            UserInfo userinfo {};
			userinfo.username =  topic[2];
			//std::string name { topic[2] + topic[3] };
			std::string site { topic[3] };
            for (std::size_t i = 4; i < topic.size(); i++) {
                site += "/" + topic[i];
            }
            userinfo.station_id = site;
            std::size_t hash { std::hash<std::string>{}(userinfo.site_id()) };

            if (m_buffer.find(hash) != m_buffer.end()) {
                auto& item { m_buffer[hash] };
                item.add(content);
                if (item.complete()) {
                    process(hash, item);
                    m_buffer.erase(hash);
                }
            } else {
                DetectorInfoCollector item;
                item.message_id = content[0];
                item.user_info = userinfo;
                item.add(content);
                m_buffer[hash] = item;
            }
        }
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

void MqttLogSource::process(std::size_t hash, DetectorInfoCollector item)
{
    static constexpr struct {
        const double pos_dop { 1.0e-1 };
        const double time_dop { 1.0e-1 };
        const double v_accuracy { 1.0e-1 };
        const double h_accuracy { 1.0e-1 };
    } factors; // norming factors for the four values

    Location location;

    location.dop = (factors.pos_dop * item.geo.dop) * (factors.time_dop * item.time.dop);
    location.prec = (factors.h_accuracy * item.geo.h_acc) * (factors.v_accuracy * item.geo.v_acc);

    location.h = item.geo.h;
    location.lat = item.geo.lat;
    location.lon = item.geo.lon;

    push_item( DetectorInfo{hash, item.user_info, location} );
}
}
