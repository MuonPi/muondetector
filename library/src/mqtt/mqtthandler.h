#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include "config.h"

// #include <QObject>
// #include <QTimer>
// #include <QPointer>
#include <string>
#include <mosquitto.h>
#include <vector>

namespace MuonPi
{

    class MqttHandler
    {
    public:
        enum class Status
        {
            Invalid,
            Connected,
            Disconnected,
            Connecting,
            Disconnecting,
            Error,
            Inhibited
        };

        MqttHandler(const std::string &station_id, const int verbosity = 0);
        ~MqttHandler();

        bool isInhibited();

    // signals:
    //     void receivedMessage(const std::string &topic, const std::string &content);
    //     void connection_status(Status status);
    //     void mqttConnect();
    //     void mqttDisconnect();

    // public slots:
        void start(const std::string &username, const std::string &password);
        void subscribe(const std::string &topic);
        void unsubscribe(const std::string &topic);
        void requestConnectionStatus();
        void setInhibited(bool inhibited = true);

    // private slots:
        void set_status(Status status);
    //     void onMqttConnect();
    //     void onMqttDisconnect();

    private:
        [[nodiscard]] auto connected() -> bool;
        [[nodiscard]] auto publish(const std::string &topic, const std::string &content) -> bool;

        void initialise(const std::string &client_id);

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
        void callback_message(const mosquitto_message *message);

        mosquitto *m_mqtt{nullptr};

        Status m_status{Status::Invalid};

        std::size_t m_tries{0};

        static constexpr std::size_t first_retry_period{Config::MQTT::retry_period.count()};
        static constexpr std::size_t max_retry_period{0x2 << Config::MQTT::max_retry_count};
        static constexpr std::size_t s_max_tries{Config::MQTT::max_retry_count};

        std::vector<std::string> m_topics{};

        std::string m_station_id{"0"};
        std::string m_username{};
        std::string m_password{};
        std::string m_client_id{};

        int m_verbose{0};

        std::size_t m_publish_error_count{0};
        static constexpr std::size_t s_max_publish_errors{3};

        friend void wrapper_callback_connected(mosquitto *mqtt, void *object, int result);
        friend void wrapper_callback_disconnected(mosquitto *mqtt, void *object, int result);
        friend void wrapper_callback_message(mosquitto *mqtt, void *object, const mosquitto_message *message);
    };
} // namespace MuonPi

#endif // MQTTHANDLER_H
