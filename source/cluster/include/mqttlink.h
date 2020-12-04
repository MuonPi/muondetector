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
 * @brief The MqttLink class. Connects to a Mqtt server and offers publish and subscribe methods.
 */
class MqttLink : public mqtt::callback
{
public:
    /**
     * @brief The Publisher class. Only gets instantiated from within the MqttLink class.
     */
    class Publisher {
    public:
        /**
         * @brief Publisher
         * @param client the mqttclient object which represents the server connection
         * @param topic The topic to connect to.
         */
        Publisher(mqtt::async_client& client, const std::string& topic);
        /**
         * @brief publish Publish a message
         * @param content The content to send
         * @return true if the sending was successful
         */
        [[nodiscard]] auto publish(const std::string& content) -> bool;
    };

    /**
     * @brief The Subscriber class. Only gets instantiated from within the MqttLink class.
     */
    class Subscriber {
    public:
        /**
         * @brief Subscriber
         * @param client the mqttclient object which represents the server connection
         * @param topic The topic to connect to.
         */
        Subscriber(mqtt::async_client& client, const std::string& topic);
        /**
         * @brief has_message Check whether there are messages available.
         * @return true if there is at least one message in the queue.
         */
        [[nodiscard]] auto has_message() const -> bool;
        /**
         * @brief get_message Gets the next message from the queue.
         * @return an std::pair containting the message
         */
        [[nodiscard]] auto get_message() -> std::pair<std::string, std::chrono::steady_clock::time_point>;
        /**
         * @brief push_message Only called from within the MqttLink class
         * @param message The message to push into the queue
         */
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

    /**
     * @brief MqttLink Create a mqttlink to a server. Creating the object starts the connection immediatly.
     * The connection happens asynchronously. To check if the connection has been established, check for the object status.
     * @param server The server to connect to
     * @param login Login information
     */
    MqttLink(const std::string& server, const LoginData& login);

    ~MqttLink() override;

    /**
     * @brief publish Create a publisher object
     * @param topic The topic over which the messages should be published
     * @return A shared_ptr to a publisher object, or nullptr in the case of failure.
     */
    [[nodiscard]] auto publish(const std::string& topic) -> std::shared_ptr<Publisher>;

    /**
     * @brief subscibe Create a Subscriber object
     * @param topic The topic for which the subscriber should listen
     * @return A shared_ptr to a subscriber object, or nullptr in the case of failure.
     */
    [[nodiscard]] auto subscribe(const std::string& topic) -> std::shared_ptr<Subscriber>;

    /**
     * @brief connected reimplemented from mqtt::callback
     * @param cause The cause of the connection?
     */
    void connected(const std::string& cause) override;
    /**
     * @brief connection_lost Reimplemented from mqtt::callback
     * @param cause The cause of the connection loss
     */
    void connection_lost(const std::string& cause) override;
    /**
     * @brief message_arrived Reimplemented from mqtt::callback
     * @param message A pointer to the message
     */
    void message_arrived(mqtt::const_message_ptr message) override;
    /**
     * @brief delivery_complete Reimplemented from mqtt::callback
     * @param token A pointer to the delivery token
     */
    void delivery_complete(mqtt::delivery_token_ptr token) override;

private:
    /**
     * @brief connects to the Server synchronuously. This method blocks until it is connected.
     * @return true if the connection was successful
     */
    [[nodiscard]] auto connect() -> bool;
    /**
     * @brief disconnect Disconnect from the server
     * @return true if the disconnect was successful
     */
    [[nodiscard]] auto disconnect() -> bool;
    /**
     * @brief reconnect attempt a reconnect after the connection was lost.
     * @return true if the reconnect was successful.
     */
    [[nodiscard]] auto reconnect() -> bool;

    /**
     * @brief set_status Sets the status for the object
     * @param status The new status of the connection
     */
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
