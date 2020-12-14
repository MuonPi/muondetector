#ifndef MQTTLOGSOURCE_H
#define MQTTLOGSOURCE_H

#include "abstractsource.h"
#include "mqttlink.h"

#include <memory>
#include <map>

namespace MuonPi {

class LogMessage;
class MessageParser;

struct LogItem {
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

    void reset();

    [[nodiscard]] auto add(MessageParser& message) -> bool;

    [[nodiscard]] auto complete() -> bool;

};


/**
 * @brief The MqttLogSource class
 */
class MqttLogSource : public AbstractSource<LogMessage>
{
public:
    /**
     * @brief MqttLogSource
     * @param subscriber The Mqtt Topic this Log source should be subscribed to
     */
    MqttLogSource(std::shared_ptr<MqttLink::Subscriber> subscriber);

    ~MqttLogSource() override;

protected:
    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

    [[nodiscard]] auto pre_run() -> int override;

private:
    std::shared_ptr<MqttLink::Subscriber> m_link { nullptr };

    void process(LogItem item);

    std::map<std::size_t, LogItem> m_buffer {};
};

}

#endif // MQTTLOGSOURCE_H
