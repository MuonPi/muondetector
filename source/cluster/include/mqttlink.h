#ifndef MQTTLINK_H
#define MQTTLINK_H

#include "threadrunner.h"

#include <string>
#include <memory>
#include <chrono>
#include <map>
#include <future>
#include <regex>
#include <queue>

#include <mosquitto.h>

namespace MuonPi {


/**
 * @brief The MqttLink class. Connects to a Mqtt server and offers publish and subscribe methods.
 */
class MqttLink : public ThreadRunner
{
public:

    enum class Status {
        Invalid,
        Connected,
        Disconnected,
        Connecting,
        Error
    };
    class BaseMessage {
	public:
		auto valid() const -> bool { return m_valid; }
		void setValid(bool valid) { m_valid = valid; }
	protected:
		bool m_valid { false };
	};
	struct Message : public BaseMessage
    {
        Message() = default;
		Message(const std::string& a_topic, const std::string& a_content) 
		: topic { a_topic }, content {a_content }
		{}
		std::string topic {};
        std::string content{};
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
    /**
     * @brief The Publisher class. Only gets instantiated from within the MqttLink class.
     */
    class Publisher {
    public:

        Publisher(MqttLink* link, const std::string& topic)
            : m_link { link }
            , m_topic { topic }
        {}
        /**
         * @brief publish Publish a message
         * @param content The content to send
         * @return true if the sending was successful
         */
        [[nodiscard]] auto publish(const std::string& content) -> bool;

        Publisher() = default;
    private:
        friend class MqttLink;

        MqttLink* m_link { nullptr };
        std::string m_topic {};
    };

    /**
     * @brief The Subscriber class. Only gets instantiated from within the MqttLink class.
     */
    class Subscriber {
    public:

        Subscriber(MqttLink* link, const std::string& topic)
            : m_link { link }
            , m_topic { topic }
        {}

        ~Subscriber()
        {
            m_link->unsubscribe(m_topic);
        }

        Subscriber() = default;
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

		[[nodiscard]] auto get_subscribe_topic() const -> const std::string&;
    private:
        friend class MqttLink;

        /**
         * @brief push_message Only called from within the MqttLink class
         * @param message The message to push into the queue
         */
        void push_message(const Message& message);

        std::queue<Message> m_messages {};
        bool m_has_message { false };
        std::mutex m_mutex;
        MqttLink* m_link { nullptr };
        std::string m_topic {};
    };



    MqttLink(const LoginData& login, const std::string& server = "muonpi.org", int port = 1883);

    ~MqttLink() override;

    void callback_connected(int result);
    void callback_disconnected(int result);
    void callback_message(const mosquitto_message* message);


    [[nodiscard]] auto publish(const std::string& topic) -> Publisher&;
    [[nodiscard]] auto subscribe(const std::string& topic) -> Subscriber&;

    [[nodiscard]] auto wait_for(Status status, std::chrono::milliseconds duration) -> bool;
protected:
    /**
     * @brief pre_run Reimplemented from ThreadRunner
     * @return 0 if the thread should start
     */
    [[nodiscard]] auto pre_run() -> int override;
    /**
     * @brief step Reimplemented from ThreadRunner
     * @return 0 if the thread should continue running
     */
    [[nodiscard]] auto step() -> int override;
    /**
     * @brief post_run Reimplemented from ThreadRunner
     * @return The return value of the thread loop
     */
    [[nodiscard]] auto post_run() -> int override;

private:

    /**
     * @brief set_status Set the status for this MqttLink
     * @param status The new status
     */
    void set_status(Status status);

    [[nodiscard]] auto publish(const std::string& topic, const std::string& content) -> bool;
    /**
     * @brief unsubscribe Unsubscribe from a specific topic
     * @param topic The topic string to unsubscribe from
     */
    void unsubscribe(const std::string& topic);
    /**
     * @brief connects to the Server synchronuously. This method blocks until it is connected.
     * @return true if the connection was successful
     */
    [[nodiscard]] auto connect(std::size_t n = 0) -> bool;
    /**
     * @brief disconnect Disconnect from the server
     * @return true if the disconnect was successful
     */
    auto disconnect() -> bool;
    /**
     * @brief reconnect attempt a reconnect after the connection was lost.
     * @return true if the reconnect was successful.
     */
    [[nodiscard]] auto reconnect(std::size_t n = 0) -> bool;


    [[nodiscard]] inline auto init(const char* client_id) -> mosquitto*
    {
        mosquitto_lib_init();
        return mosquitto_new(client_id, true, this);
    }

    std::string m_host {};
    int m_port { 1883 };
    LoginData m_login_data {};
    mosquitto *m_mqtt { nullptr };

    Status m_status { Status::Invalid };

    std::map<std::string, std::unique_ptr<Publisher>> m_publishers {};
    std::map<std::string, std::unique_ptr<Subscriber>> m_subscribers {};

    std::size_t m_tries { 0 };
};

void wrapper_callback_connected(mosquitto* mqtt, void* object, int result);
void wrapper_callback_disconnected(mosquitto* mqtt, void* object, int result);
void wrapper_callback_message(mosquitto* mqtt, void* object, const mosquitto_message* message);



}

#endif // MQTTLINK_H
