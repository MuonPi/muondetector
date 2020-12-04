#ifndef MQTTLINK_H
#define MQTTLINK_H

#include <string>
#include <memory>
#include <chrono>
#include <map>
#include <future>

#include <mqtt/async_client.h>

namespace MuonPi {

/**
 * @brief The MqttLink class
 */
class MqttLink : public mqtt::callback
{
public:
    class Publisher {
    public:
        Publisher(mqtt::async_client& client, const std::string& topic);
        [[nodiscard]] auto publish(const std::string& content) -> bool;
    };

    class Subscriber {
    public:
        Subscriber(mqtt::async_client& client, const std::string& topic);
        [[nodiscard]] auto has_message() const -> bool;
        [[nodiscard]] auto get_message() -> std::pair<std::string, std::chrono::steady_clock::time_point>;
        void push_message(std::pair<std::string, std::chrono::steady_clock::time_point> message);
    private:
        std::queue<std::pair<std::string, std::chrono::steady_clock::time_point>> m_messages {};
    };

    struct LoginData
    {
        std::string username {};
        std::string station_id {};
        std::string client_id {};
        std::string password {};
    };

    enum class Status {
        Invalid,
        Connected,
        Disconnected,
        Connecting,
        Error
    };

    MqttLink(const std::string& server, const LoginData& login);

    ~MqttLink() override;

    [[nodiscard]] auto publish(const std::string& topic) -> std::shared_ptr<Publisher>;
    [[nodiscard]] auto subscribe(const std::string& topic) -> std::shared_ptr<Subscriber>;

    void connected(const std::string& cause) override;
    void connection_lost(const std::string& cause) override;
    void message_arrived(mqtt::const_message_ptr message) override;
    void delivery_complete(mqtt::delivery_token_ptr token) override;

private:
    [[nodiscard]] auto connect() -> bool;
    [[nodiscard]] auto disconnect() -> bool;
    [[nodiscard]] auto reconnect() -> bool;

    void set_status(Status status);

    std::string m_server {};
    LoginData m_login_data {};

    Status m_status { Status::Invalid };

    std::map<std::string, std::shared_ptr<Publisher>> m_publishers {};
    std::map<std::string, std::shared_ptr<Subscriber>> m_subscribers {};

    std::unique_ptr<mqtt::async_client> m_client { nullptr };
    mqtt::connect_options m_conn_options {};

    std::future<bool> m_connection_status {};
};

}

#endif // MQTTLINK_H
