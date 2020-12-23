#ifndef MQTTLOGSOURCE_H
#define MQTTLOGSOURCE_H

#include "abstractsource.h"
#include "mqttlink.h"
#include "detectorinfo.h"
#include "userinfo.h"
#include "utility.h"
#include "log.h"

#include <map>
#include <memory>
#include <map>

namespace MuonPi {

//class DetectorInfo;
//class MessageParser;

class AbstractMqttItemCollector {
public:
    UserInfo user_info {};
    std::string message_id {};

    virtual ~AbstractMqttItemCollector();

    /**
     * @brief reset Resets the ItemCollector to its default state
     */
    void reset();

    /**
     * @brief add Tries to add a Message to the Item. The item chooses which messages to keep
     * @param message The message to pass
     * @return true if the Message was accepted. False in an error or if the message was not accepted
     */
    virtual auto add(MessageParser& message) -> bool;

    /**
     * @brief complete indicates whether the Item has collected all required messages
     * @return true if it is complete
     */
    [[nodiscard]] auto complete() -> bool;

protected:
    std::uint16_t m_status { 0 };
    std::uint16_t default_status { 0x00FF };
};

AbstractMqttItemCollector::~AbstractMqttItemCollector() = default;

/**
 * @brief Helper class for complete collection of several logically connected DetectorLogItems
 */
class DetectorInfoCollector : public AbstractMqttItemCollector {
public:
    //UserInfo user_info {};
    //std::string message_id {};
    //std::uint8_t status { s_default_status };

    struct {
        double h;
        double lat;
        double lon;
        double h_acc;
        double v_acc;
        double dop;
    } geo;

    struct {
        double accuracy;
        double dop;
    } time;

    virtual ~DetectorInfoCollector() override = default;

        /**
     * @brief add Tries to add a Message to the Item. The item chooses which messages to keep
     * @param message The message to pass
     * @return true if the Message was accepted. False in an error or if the message was not accepted
     */
    auto add(MessageParser& message) -> bool override;
private:
};

/**
 * @brief The MqttLogSource class
 */
template <class T>
class MqttLogSource : public AbstractSource<T>
{
public:
    /**
     * @brief MqttLogSource
     * @param subscriber The Mqtt Topic this Log source should be subscribed to
     */
    MqttLogSource(MqttLink::Subscriber& subscriber);

    ~MqttLogSource() override;


protected:
    /**
     * @brief pre_run Reimplemented from ThreadRunner
     * @return 0 if it should continue to run
     */
    auto pre_run() -> int override;

    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:
    /**
     * @brief process Processes one LogItem
     * @param hash The hash of the detector
     * @param item The item to process
     */
    //void process(std::size_t hash, DetectorInfoCollector item);
    void process(const MqttLink::Message& msg);

    MqttLink::Subscriber& m_link;

    //std::map<std::size_t, MqttLink::Message> m_msg_buffer {};
    std::map<std::size_t, std::unique_ptr<AbstractMqttItemCollector>> m_buffer {};
};


// +++++++++++++++++++++++++++++++
// implementation part starts here
// +++++++++++++++++++++++++++++++

void AbstractMqttItemCollector::reset() {
    user_info = UserInfo { };
    m_status = default_status;
}

auto AbstractMqttItemCollector::complete() -> bool
{
    return !m_status;
}

auto AbstractMqttItemCollector::add(MessageParser& /*message*/) -> bool
{
	return false;
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
            m_status &= ~1;
        } else if (message[1] == "geoHorAccuracy") {
            geo.h_acc = std::stod(message[2], nullptr);
            m_status &= ~2;
        } else if (message[1] == "geoLatitude") {
            geo.lat = std::stod(message[2], nullptr);
            m_status &= ~4;
        } else if (message[1] == "geoLongitude") {
            geo.lon = std::stod(message[2], nullptr);
            m_status &= ~8;
        } else if (message[1] == "geoVertAccuracy") {
            geo.v_acc = std::stod(message[2], nullptr);
            m_status &= ~16;
        } else if (message[1] == "positionDOP") {
            geo.dop = std::stod(message[2], nullptr);
            m_status &= ~32;
        } else if (message[1] == "timeAccuracy") {
            time.accuracy = std::stod(message[2], nullptr);
            m_status &= ~64;
        } else if (message[1] == "timeDOP") {
            time.dop = std::stod(message[2], nullptr);
            m_status &= ~128;
        } else {
            return false;
        }
    } catch(std::invalid_argument& e) {
        Log::warning()<<"received exception when parsing log item: " + std::string(e.what());
        return false;
    }

    return true;
}


template <class T>
MqttLogSource<T>::MqttLogSource(MqttLink::Subscriber& subscriber)
    : m_link { subscriber }
{
    AbstractSource<T>::start();
}

template <class T>
MqttLogSource<T>::~MqttLogSource() = default;

template <class T>
auto MqttLogSource<T>::pre_run() -> int
{
    return 0;
}

template <class T>
auto MqttLogSource<T>::step() -> int
{
    if (m_link.has_message()) {
        MqttLink::Message msg = m_link.get_message();
        this->process(msg);
    }
    std::this_thread::sleep_for(std::chrono::microseconds{50});
    return 0;
}

template <>
void MqttLogSource<MqttLink::Message>::process(const MqttLink::Message& msg)
{
    push_item( MqttLink::Message { msg } );
}

template <>
void MqttLogSource<DetectorInfo>::process(const MqttLink::Message& msg)
{
        MessageParser topic { msg.topic, '/'};
        MessageParser content { msg.content, ' '};
        MessageParser subscribe_topic { m_link.get_subscribe_topic(), '/' };

        if ( topic[2] == "" || topic[2] == "cluster"  ) return;

        if ((topic.size() >= 4) && (content.size() >= 2)) {
            UserInfo userinfo {};
            userinfo.username =  topic[2];
            std::string site { topic[3] };
            for (std::size_t i = 4; i < topic.size(); i++) {
                site += "/" + topic[i];
            }
            userinfo.station_id = site;
            std::size_t hash { std::hash<std::string>{}(userinfo.site_id()) };

            if (m_buffer.find(hash) != m_buffer.end()) {
//                DetectorInfoCollector& item = dynamic_cast<DetectorInfoCollector&>( static_cast<std::unique_ptr<DetectorInfoCollector>>(m_buffer[hash]) );
                DetectorInfoCollector* item { dynamic_cast<DetectorInfoCollector*>( m_buffer[hash].get() ) };
                item->add(content);

                if (item->complete()) {
                    static constexpr struct {
                        const double pos_dop { 1.0e-1 };
                        const double time_dop { 1.0e-1 };
                        const double v_accuracy { 1.0e-1 };
                        const double h_accuracy { 1.0e-1 };
                    } factors; // norming factors for the four values

                    Location location;

                    location.dop = (factors.pos_dop * item->geo.dop) * (factors.time_dop * item->time.dop);
                    location.prec = (factors.h_accuracy * item->geo.h_acc) * (factors.v_accuracy * item->geo.v_acc);

                    location.h = item->geo.h;
                    location.lat = item->geo.lat;
                    location.lon = item->geo.lon;

                    this->push_item( DetectorInfo{hash, item->user_info, location} );
                    //process(hash, item);
                    m_buffer.erase(hash);
                }
            } else {
                DetectorInfoCollector item;
                item.message_id = content[0];
                item.user_info = userinfo;
                item.add(content);
				m_buffer.insert( { hash,  std::unique_ptr<AbstractMqttItemCollector>( new DetectorInfoCollector(item) ) } );
            }
        }
}
}

#endif // MQTTLOGSOURCE_H
