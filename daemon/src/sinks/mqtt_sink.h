#ifndef MQTT_SINK_H
#define MQTT_SINK_H

#include "config.h"
#include "core/event_bus.h"
#include "data/events/mqtt_log_event.h"
#include "data/events/mqtt_status_event.h"
#include "data/events/ubx_event.h"
#include "sinks/sink.h"

#include <atomic>

class mosquitto;
struct mosquitto_message;
class MqttSink : public Sink {
  public:
    explicit MqttSink(EventBus& bus, const std::string& station_id);
    ~MqttSink();

    bool isInhibited();
    void handle(const UbxTimeMarkStruct& tm);
    void handle(const MqttLogEvent& event);
    void shutdown() override;
    void start(const std::string& username, const std::string& password);
    auto connectionStatus() const -> MqttStatusEvent::Status;

  private:
    [[nodiscard]] auto connected() -> bool;
    auto publish(const std::string& topic, const std::string& content) -> bool;

    void initialise(const std::string& client_id);

    void mqttConnect();
    void mqttDisconnect();
    void subscribe(const std::string& topic);
    void unsubscribe(const std::string& topic);

    void requestConnectionStatus();

    void cleanup();

    /**
     * @brief callback_connected Gets called by mosquitto client
     * @param result The status code from the callback
     */
    void callback_connected(int result);

    /**
     * @brief callback_disconnected Gets called by mosquitto client
     * @param result The status code from the callback
     */
    void callback_disconnected(int result);

    /**
     * @brief callback_message Gets called by mosquitto client in the case of an arriving message
     * @param message A const pointer to the received message
     */
    void callback_message(const mosquitto_message* message);

    void setStatus(MqttStatusEvent::Status status);
    void setInhibited(bool inhibited);

    mosquitto* m_mqtt{nullptr};

    std::atomic<MqttStatusEvent::Status> m_status{MqttStatusEvent::Status::Invalid};

    std::size_t m_tries{0};

    static constexpr std::size_t first_retry_period{MuonPi::Config::MQTT::retry_period.count()};
    static constexpr std::size_t max_retry_period{0x2 << MuonPi::Config::MQTT::max_retry_count};
    static constexpr std::size_t s_max_tries{MuonPi::Config::MQTT::max_retry_count};

    std::vector<std::string> m_topics{};

    EventBus& bus_;

    std::string m_station_id{"0"};
    std::string m_username{};
    std::string m_password{};
    std::string m_client_id{};

    std::atomic<std::size_t> m_publish_error_count{0};
    static constexpr std::size_t s_max_publish_errors{3};

    friend void wrapper_callback_connected(mosquitto* mqtt, void* object, int result);
    friend void wrapper_callback_disconnected(mosquitto* mqtt, void* object, int result);
    friend void wrapper_callback_message(mosquitto* mqtt, void* object,
                                         const mosquitto_message* message);
};
#endif // MQTT_SINK_H
