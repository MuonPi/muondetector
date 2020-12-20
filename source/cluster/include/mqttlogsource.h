#ifndef MQTTLOGSOURCE_H
#define MQTTLOGSOURCE_H

#include "abstractsource.h"
#include "mqttlink.h"
#include "detectorinfo.h"

#include <map>
#include <memory>
#include <map>

namespace MuonPi {

class DetectorInfo;
class MessageParser;

struct DetectorLogItem {
    static constexpr std::uint8_t s_default_status { 0xFF };
    std::string id {};
    std::uint8_t status { s_default_status };

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

    /**
     * @brief reset Resets the LogItem to its default state
     */
    void reset();

    /**
     * @brief add Tries to add a Message to the Item. The item chooses which messages to keep
     * @param message The message to pass
     * @return true if the Message was accepted. False in an error or if the message was not accepted
     */
    auto add(MessageParser& message) -> bool;

    /**
     * @brief complete indicates whether the Item has collected all required messages
     * @return true if it is complete
     */
    [[nodiscard]] auto complete() -> bool;

};

/**
 * @brief The MqttLogSource class
 */
class MqttLogSource : public AbstractSource<DetectorInfo>
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
    void process(std::size_t hash, DetectorLogItem item);

    MqttLink::Subscriber& m_link;

    std::map<std::size_t, DetectorLogItem> m_buffer {};
};

}

#endif // MQTTLOGSOURCE_H
