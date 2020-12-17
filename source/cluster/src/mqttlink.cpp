#include "mqttlink.h"
#include "log.h"

#include <functional>
#include <sstream>
#include <regex>

#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

namespace MuonPi {

MqttLink::MqttLink(const std::string& server, const LoginData& login)
    : ThreadRunner{"MqttLink"}
    , m_server { server }
    , m_login_data { login }
    , m_client { server, login.client_id() }
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

auto MqttLink::startup() -> bool
{
    if (!wait_for(MuonPi::MqttLink::Status::Connected, std::chrono::seconds{5})) {
        return false;
    }
    return true;
}


auto MqttLink::wait_for(Status status, std::chrono::seconds duration) -> bool
{
    std::chrono::system_clock::time_point start { std::chrono::system_clock::now() };
    while (status != m_status) {
        std::this_thread::sleep_for(duration / 10);
        if ((std::chrono::system_clock::now() - start) >= duration) {
            return false;
        }
    }
    return true;
}

auto MqttLink::pre_run() -> int
{
    if (m_connection_status.valid()) {
        if (!m_connection_status.get()) {
            return -1;
        }
    }
    if (m_status != Status::Connected) {
        return -1;
    }
    m_client.start_consuming();
    return 0;
}

auto MqttLink::step() -> int
{
    if (!m_client.is_connected()) {
        if (!reconnect()) {
            return -1;
        }
    }
    mqtt::const_message_ptr message;
    if (m_client.try_consume_message(&message)) {
        std::string message_topic {message->get_topic()};
        for (auto& [topic, subscriber]: m_subscribers) {
            std::regex regex{topic};
            std::smatch match;
            if (std::regex_match(message_topic, match, regex)) {
                subscriber->push_message({message_topic, message->to_string()});
            }
        }
    }
    std::this_thread::sleep_for(std::chrono::microseconds{10});
    return 0;
}

auto MqttLink::post_run() -> int
{
    for (auto& subscriber : m_subscribers) {
        subscriber.second->unsubscribe();
    }

    m_subscribers.clear();
    m_publishers.clear();
    m_client.stop_consuming();
    if (!disconnect()) {
        return -1;
    }
    return 0;
}

auto MqttLink::publish(const std::string& topic) -> Publisher&
{
    if (m_status != Status::Connected) {
        throw -1;
    }
    if (m_publishers.find(topic) != m_publishers.end()) {
        throw -1;
    }
    m_publishers.emplace(std::make_pair(topic, std::make_unique<Publisher>(m_client, topic)));
    Log::debug()<<"Starting to publish on topic " + topic;
    return *m_publishers[topic];
}

auto MqttLink::subscribe(const std::string& topic, const std::string& regex) -> Subscriber&
{
    if (m_status != Status::Connected) {
        throw -1;
    }
    std::string check_topic { topic};
    if (m_subscribers.find(regex) != m_subscribers.end()) {
        throw -1;
    }
    m_subscribers.emplace(std::make_pair(regex, std::make_unique<Subscriber>(m_client, topic)));
    Log::debug()<<"Starting to subscribe to topic " + topic;
    return *m_subscribers[regex];
}

auto MqttLink::connect() -> bool
{
    set_status(Status::Connecting);
    static constexpr std::size_t max_tries { 5 };
    static std::size_t n { 0 };

    if (n > max_tries) {
        set_status(Status::Error);
        Log::error()<<"Giving up trying to connect to MQTT.";
        return false;
    }
    try {
        m_client.connect(m_conn_options)->wait();
        set_status(Status::Connected);
        n = 0;
        Log::info()<<"Connected to MQTT";
        return true;
    } catch (const mqtt::exception& exc) {
        std::this_thread::sleep_for( std::chrono::seconds{1} );
        n++;
        Log::warning()<<"Received exception while tryig to connect to MQTT, but retrying: " + std::string{exc.what()};
        return connect();
    }
}

auto MqttLink::disconnect() -> bool
{
    if (m_status != Status::Connected) {
        return true;
    }
    try {
        m_client.disconnect()->wait();
        set_status(Status::Disconnected);
        Log::info()<<"Disconnected from MQTT.";
        return true;
    } catch (const mqtt::exception& exc) {
        Log::error()<<"Received exception while tryig to disconnect from MQTT: " + std::string{exc.what()};
        return false;
    }
}

auto MqttLink::reconnect() -> bool
{
    set_status(Status::Disconnected);
    static constexpr std::size_t max_tries { 5 };
    static std::size_t n { 0 };

    if (n > max_tries) {
        set_status(Status::Error);
        Log::error()<<"Giving up trying to reconnect to MQTT.";
        return false;
    }

    Log::info()<<"Trying to reconnect to MQTT.";
    try {
        if (!m_client.reconnect()->wait_for(std::chrono::seconds{5})) {
            Log::error()<<"Could not reconnect to MQTT.";
            n++;
            return reconnect();
        }
        set_status(Status::Connected);
        Log::info()<<"Connected to MQTT";
        n = 0;
        return true;
    } catch (const mqtt::exception& exc) {
        Log::error()<<"Received exception while tryig to reconnect from MQTT: " + std::string{exc.what()};
        n++;
        return reconnect();
    }
}

void MqttLink::set_status(Status status) {
    m_status = status;
}

MqttLink::Publisher::Publisher(mqtt::async_client& client, const std::string& topic)
    : m_client { client }
    , m_topic { topic }
{
}

auto MqttLink::Publisher::publish(const std::string& content) -> bool
{
    try {
        m_client.publish(m_topic, content);
        return true;
    } catch (const mqtt::exception& exc) {
        Log::error()<<"Received exception while tryig to publish MQTT message: " + std::string{exc.what()};
        return false;
    }
}


MqttLink::Subscriber::Subscriber(mqtt::async_client& client, const std::string& topic)
    : m_client { client }
    , m_topic { topic }
{
    auto token {m_client.subscribe(m_topic, 1)};
    token->wait();
}

void MqttLink::Subscriber::unsubscribe()
{
    m_client.unsubscribe(m_topic)->wait();
}

auto MqttLink::Subscriber::has_message() const -> bool
{
    return !m_messages.empty();
}

auto MqttLink::Subscriber::get_message() -> Message
{
    std::scoped_lock<std::mutex> lock {m_mutex};
    if (m_messages.empty()) {
        return {};
    }
    auto msg { m_messages.front() };
    m_messages.pop();
    return msg;
}

void MqttLink::Subscriber::push_message(const Message &message)
{
    std::scoped_lock<std::mutex> lock {m_mutex};
    m_messages.push(message);
}

auto MqttLink::LoginData::client_id() const -> std::string
{
    CryptoPP::SHA1 sha1;

    std::string source {username + station_id};
    std::string id {};
    CryptoPP::StringSource{source, true, new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(new CryptoPP::StringSink(id)))};
    return id;
}
}
