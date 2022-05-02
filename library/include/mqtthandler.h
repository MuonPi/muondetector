#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include "muondetector_shared_global.h"
#include "config.h"

#include <QObject>
#include <QTimer>
#include <QPointer>
#include <string>
#include <mosquitto.h>

namespace MuonPi {

class MUONDETECTORSHARED MqttHandler : public QObject
{
    Q_OBJECT

public:
    enum class Status {
        Invalid,
        Connected,
        Disconnected,
        Connecting,
        Disconnecting,
        Error,
        Inhibited
    };

    MqttHandler(const QString& station_id, const int verbosity=0);
    ~MqttHandler() override;

    void mqttStartConnection();
    bool isInhibited();

signals:
    void mqttConnectionStatus(bool connected);
    void receivedMessage(const QString& topic, const QString& content);
    void giving_up();
    void connection_status(Status status);
    void mqttConnect();
    void mqttDisconnect();

public slots:
    void start(const QString& username, const QString& password);
    void subscribe(const QString& topic);
    void unsubscribe(const QString& topic);
    void publish(const QString& topic, const QString& content);
    void onRequestConnectionStatus();
    void setInhibited(bool inhibited = true);

private slots:
    void set_status(Status status);
    void onMqttConnect();
    void onMqttDisconnect();
    void error_handler();

private:
    [[nodiscard]] auto connected() -> bool;
    [[nodiscard]] auto publish(const std::string& topic, const std::string& content) -> bool;

    void initialise(const std::string& client_id);

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

    mosquitto* m_mqtt { nullptr };

    Status m_status { Status::Invalid };

    std::size_t m_tries { 0 };

    static constexpr std::size_t s_max_tries { Config::MQTT::max_retry_count };

    std::vector<std::string> m_topics {};

    bool m_mqttConnectionStatus { false };
    std::string m_station_id { "0" };
    std::string m_username {};
    std::string m_password {};
    std::string m_client_id {};

    int m_verbose { 0 };
    
    std::size_t m_publish_error_count { 0 };
    static constexpr std::size_t s_max_publish_errors { 3 };

    friend void wrapper_callback_connected(mosquitto* mqtt, void* object, int result);
    friend void wrapper_callback_disconnected(mosquitto* mqtt, void* object, int result);
    friend void wrapper_callback_message(mosquitto* mqtt, void* object, const mosquitto_message* message);
};
} // namespace MuonPi

#endif // MQTTHANDLER_H
