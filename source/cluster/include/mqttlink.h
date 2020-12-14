#ifndef MQTTLINK_H
#define MQTTLINK_H

#include "threadrunner.h"

#include <string>
#include <memory>
#include <chrono>
#include <map>
#include <future>
#include <regex>

#include <mqtt/async_client.h>

namespace MuonPi {

/**
 * @brief The MqttLink class. Connects to a Mqtt server and offers publish and subscribe methods.
 */
class MqttLink : public ThreadRunner
{
public:
    struct Message
    {
        std::string topic {};
        std::string content{};
    };
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
    private:
        mqtt::async_client& m_client;
        std::string m_topic {};
        bool m_valid { true };
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

        Subscriber();

        /**
         * @brief has_message Check whether there are messages available.
         * @return true if there is at least one message in the queue.
         */
        [[nodiscard]] auto has_message() const -> bool;
        /**
         * @brief get_message Gets the next message from the queue.
         * @return an std::pair containting the message
         */
        [[nodiscard]] auto get_message() -> Message;

    private:
        friend class MqttLink;
        /**
         * @brief push_message Only called from within the MqttLink class
         * @param message The message to push into the queue
         */
        void push_message(const Message& message);


        void unsubscribe();


        std::queue<Message> m_messages {};
        std::mutex m_mutex {};

        mqtt::async_client& m_client;
        std::string m_topic {};

        bool m_valid { true };
    };

    struct LoginData
    {
        std::string username {};
        std::string station_id {};
        std::string password {};

        /**
         * @brief client_id Creates a client_id from the username and the station id.
         * This hashes the concatenation of the two fields.
         * @return The client id as string
         */
        [[nodiscard]] auto client_id() const -> std::string;
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

    void startup();

    /**
     * @brief publish Create a publisher object
     * @param topic The topic over which the messages should be published
     * @return A shared_ptr to a publisher object, or nullptr in the case of failure.
     */
    [[nodiscard]] auto publish(const std::string& topic) -> Publisher&;

    /**
     * @brief subscibe Create a Subscriber object
     * @param topic The topic for which the subscriber should listen
     * @return A shared_ptr to a subscriber object, or nullptr in the case of failure.
     */
    [[nodiscard]] auto subscribe(const std::string& topic, const std::string& regex) -> Subscriber&;

    [[nodiscard]] auto wait_for(Status status, std::chrono::seconds duration) -> bool;

protected:
    [[nodiscard]] auto pre_run() -> int override;
    [[nodiscard]] auto step() -> int override;
    [[nodiscard]] auto post_run() -> int override;
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

    std::map<std::string, std::unique_ptr<Publisher>> m_publishers {};
    std::map<std::string, std::unique_ptr<Subscriber>> m_subscribers {};

    mqtt::async_client m_client;
    mqtt::connect_options m_conn_options {};

    std::future<bool> m_connection_status {};
};

}

#endif // MQTTLINK_H
