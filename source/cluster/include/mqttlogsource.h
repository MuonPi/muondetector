#ifndef MQTTLOGSOURCE_H
#define MQTTLOGSOURCE_H

#include "abstractsource.h"
#include "mqttlink.h"
#include "logmessage.h"

#include <map>
#include <memory>

namespace MuonPi {

//class LogMessage;

struct PartialLogEntry {
	LogMessage logmessage { LogMessage(std::size_t {}, Location {}) };
	std::uint8_t completeness {0x00};
	static constexpr std::uint8_t max_completeness { 0x1f };
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

	auto pre_run() -> int override;

protected:
    /**
     * @brief step implementation from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:
    std::shared_ptr<MqttLink::Subscriber> m_link { nullptr };
//	std::map<std::size_t, LogMessage> m_logbuffer { }; 
	std::map<std::size_t, PartialLogEntry> m_logbuffer { }; 
};

}

#endif // MQTTLOGSOURCE_H
