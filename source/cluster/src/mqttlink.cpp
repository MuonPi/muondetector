#include "mqttlink.h"

#include <functional>
#include <sstream>

namespace MuonPi {

MqttLink::MqttLink(const std::string& server, const LoginData& login)
    : m_server { server }
    , m_login_data { login }
    , m_client { std::make_unique<mqtt::async_client>(server, login.client_id) }
{
    m_conn_options.set_user_name(login.username);
    m_conn_options.set_password(login.password);
//    m_conn_options.set_keep_alive_interval(Config::MQTT::keepalive_interval);

    m_connection_status = std::async(std::launch::async, &MqttLink::connect, this);
}

MqttLink::~MqttLink()
{
    if (!disconnect()) {
    }
}

auto MqttLink::publish(const std::string& topic) -> std::shared_ptr<Publisher>
{
    if (m_status != Status::Connected) {
        return nullptr;
    }
    if (m_publishers.find(topic) != m_publishers.end()) {
        return nullptr;
    }
    m_publishers[topic] = std::make_shared<Publisher>(*m_client, topic);
    return m_publishers[topic];
}

auto MqttLink::subscribe(const std::string& topic) -> std::shared_ptr<Subscriber>
{
    if (m_status != Status::Connected) {
        return nullptr;
    }
    if (m_subscribers.find(topic) != m_subscribers.end()) {
        return nullptr;
    }
    m_subscribers[topic] = std::make_shared<Subscriber>(*m_client, topic);
    return m_subscribers[topic];
}

auto MqttLink::connect() -> bool
{
    set_status(Status::Connecting);
    static constexpr std::size_t max_tries { 5 };
    static std::size_t n { 0 };

    if (n > max_tries) {
        set_status(Status::Error);
        return false;
    }
    try {
        m_client->connect(m_conn_options)->wait();
        set_status(Status::Connected);
        m_client->set_callback(*this);
        n = 0;
        return true;
    } catch (const mqtt::exception& /*exc*/) {
        std::this_thread::sleep_for( std::chrono::seconds{1} );
        n++;
        return connect();
    }
}

auto MqttLink::disconnect() -> bool
{
    if (m_status != Status::Connected) {
        return true;
    }
    try {
        m_client->disconnect()->wait();
        set_status(Status::Disconnected);
        return true;
    } catch (const mqtt::exception& /*exc*/) {
        return false;
    }
}

auto MqttLink::reconnect() -> bool
{
    set_status(Status::Disconnected);
    try {
        if (!m_client->reconnect()->wait_for(std::chrono::seconds{5})) {
            return false;
        }
        set_status(Status::Connected);
        return true;
    } catch (const mqtt::exception& /*exc*/) {
        return false;
    }
}

void MqttLink::set_status(Status status) {
    m_status = status;
}

void MqttLink::connected(const std::string& /*cause*/)
{

}

void MqttLink::connection_lost(const std::string& /*cause*/)
{

}

void MqttLink::message_arrived(mqtt::const_message_ptr message)
{
    auto now { std::chrono::steady_clock::now() };
    auto subscriber { m_subscribers.find(message->get_topic()) };
    if (subscriber == m_subscribers.end()) {
        return;
    }
    (*subscriber).second->push_message(std::make_pair(message->to_string(), now));
}

void MqttLink::delivery_complete(mqtt::delivery_token_ptr /*token*/)
{
}


MqttLink::Publisher::Publisher(mqtt::async_client& client, const std::string& topic)
    : m_topic{client, topic}
{
}

auto MqttLink::Publisher::publish(const std::string& content) -> bool
{
    try {
        m_topic.publish(content);
        return true;
    }  catch (...) {
        return false;
    }
}


MqttLink::Subscriber::Subscriber(mqtt::async_client& client, const std::string& topic)
    : m_topic { client, topic}
{
    m_topic.subscribe();
}

auto MqttLink::Subscriber::has_message() const -> bool
{
    return !m_messages.empty();
}

auto MqttLink::Subscriber::get_message() -> std::pair<std::string, std::chrono::steady_clock::time_point>
{
    std::scoped_lock<std::mutex> lock {m_mutex};
    if (m_messages.empty()) {
        return {};
    }
    auto msg { m_messages.front() };
    m_messages.pop();
    return msg;
}

void MqttLink::Subscriber::push_message(std::pair<std::string, std::chrono::steady_clock::time_point> message)
{
    std::scoped_lock<std::mutex> lock {m_mutex};
    m_messages.push(message);
}
}
