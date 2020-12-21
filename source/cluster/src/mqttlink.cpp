#include "mqttlink.h"
#include "log.h"

#include <functional>
#include <sstream>
#include <regex>

#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>

namespace MuonPi {


auto MqttLink::wait_for(Status status, std::chrono::milliseconds duration) -> bool
{
    std::chrono::steady_clock::time_point start { std::chrono::steady_clock::now() };
    while (((std::chrono::steady_clock::now() - start) < duration)) {
        if (m_status == status) {
            return true;
        }
    }
    return false;
}

void wrapper_callback_connected(mosquitto* /*mqtt*/, void* object, int result)
{
    reinterpret_cast<MqttLink*>(object)->callback_connected(result);
}

void wrapper_callback_disconnected(mosquitto* /*mqtt*/, void* object, int result)
{
    reinterpret_cast<MqttLink*>(object)->callback_disconnected(result);
}

void wrapper_callback_message(mosquitto* /*mqtt*/, void* object, const mosquitto_message* message)
{
    reinterpret_cast<MqttLink*>(object)->callback_message(message);
}

MqttLink::MqttLink(const LoginData& login, const std::string& server, int port)
    : ThreadRunner{"MqttLink"}
    , m_host { server }
    , m_port { port }
    , m_login_data { login }
    , m_mqtt { init(login.client_id().c_str()) }
{
    mosquitto_connect_callback_set(m_mqtt, wrapper_callback_connected);
    mosquitto_disconnect_callback_set(m_mqtt, wrapper_callback_disconnected);
    mosquitto_message_callback_set(m_mqtt, wrapper_callback_message);

    start();
}

MqttLink::~MqttLink()
{
    disconnect();
    if (m_mqtt != nullptr) {
        mosquitto_destroy(m_mqtt);
        m_mqtt = nullptr;
    }
    mosquitto_lib_cleanup();
}

auto MqttLink::pre_run() -> int
{
    if (!connect()) {
        return -1;
    }
    if ((m_status != Status::Connected) && (m_status != Status::Connecting)) {
        return -1;
    }
    return 0;
}

auto MqttLink::step() -> int
{
    if ((m_status != Status::Connected) && (m_status != Status::Connecting)) {
        if (!reconnect()) {
            return -1;
        }
    }
    auto status = mosquitto_loop(m_mqtt, 0, 1);
    if (status != MOSQ_ERR_SUCCESS) {
        switch (status) {
        case MOSQ_ERR_INVAL:
            Log::error()<<"MqttLink could not execute step: invalid";
            return -1;
        case MOSQ_ERR_NOMEM:
            Log::error()<<"MqttLink could not execute step: memory exceeded";
            return -1;
        case MOSQ_ERR_NO_CONN:
            Log::error()<<"MqttLink could not execute step: not connected";
            if (!connect()) {
                return -1;
            }
            break;
        case MOSQ_ERR_CONN_LOST:
            Log::error()<<"MqttLink could not execute step: lost connection";
            if (!reconnect()) {
                return -1;
            }
            break;
        case MOSQ_ERR_PROTOCOL:
            Log::error()<<"MqttLink could not execute step: protocol error";
            return -1;
        case MOSQ_ERR_ERRNO:
            Log::error()<<"MqttLink could not execute step: system call error";
            return -1;
        }
    }

    std::this_thread::sleep_for(std::chrono::microseconds{10});
    return 0;
}

void MqttLink::callback_connected(int result)
{
    if (result == 1) {
        Log::warning()<<"Mqtt connection failed: Wrong protocol version";
        set_status(Status::Error);
    } else if (result == 2) {
        Log::warning()<<"Mqtt connection failed: Credentials rejected";
        set_status(Status::Error);
    } else if (result == 3) {
        Log::warning()<<"Mqtt connection failed: Broker unavailable";
        set_status(Status::Error);
    } else if (result > 3) {
        Log::warning()<<"Mqtt connection failed: Other reason";
    } else if (result == 0) {
        Log::info()<<"Connected to mqtt.";
        set_status(Status::Connected);
        m_tries = 0;
        return;
    }
}

void MqttLink::callback_disconnected(int result)
{
    if (result != 0) {
        Log::warning()<<"Mqtt disconnected unexpectedly.";
        set_status(Status::Error);
    } else {
        set_status(Status::Disconnected);
    }
}

void MqttLink::callback_message(const mosquitto_message* message)
{
    std::string message_topic {message->topic};
    for (auto& [topic, subscriber]: m_subscribers) {
        bool result { };
        mosquitto_topic_matches_sub2(topic.c_str(), topic.length(), message_topic.c_str(), message_topic.length(), &result);
        if (result) {
            subscriber->push_message({message_topic, std::string{reinterpret_cast<char*>(message->payload)}});
        }
    }
}

auto MqttLink::post_run() -> int
{
    m_subscribers.clear();
    m_publishers.clear();

    if (!disconnect()) {
        return -1;
    }
    return 0;
}

auto MqttLink::publish(const std::string& topic, const std::string& content) -> bool
{
    if (m_status != Status::Connected) {
        return false;
    }
    auto result { mosquitto_publish(m_mqtt, nullptr, topic.c_str(), static_cast<int>(content.size()), reinterpret_cast<const void*>(content.c_str()), 1, false) };
    if (result == MOSQ_ERR_SUCCESS) {
        return true;
    }
    Log::warning()<<"Could not send Mqtt message: " + std::to_string(result);
    return false;
}

void MqttLink::unsubscribe(const std::string& topic)
{
    if (m_status != Status::Connected) {
        return;
    }
    Log::info()<<"Unsubscribing from " + topic;
    mosquitto_unsubscribe(m_mqtt, nullptr, topic.c_str());
}


auto MqttLink::publish(const std::string& topic) -> Publisher&
{
    if (m_status != Status::Connected) {
        throw -1;
    }
    if (m_publishers.find(topic) != m_publishers.end()) {
        throw -1;
    }
    m_publishers[topic] = std::make_unique<Publisher>(this, topic);
    Log::debug()<<"Starting to publish on topic " + topic;
    return *m_publishers[topic];
}

auto MqttLink::subscribe(const std::string& topic) -> Subscriber&
{
    if (m_status != Status::Connected) {
        Log::warning()<<"MqttLink not connected.";
        throw -1;
    }
    std::string check_topic { topic};
    if (m_subscribers.find(topic) != m_subscribers.end()) {
        Log::info()<<"Topic already subscribed.";
        return *m_subscribers[topic];
    }
    auto result { mosquitto_subscribe(m_mqtt, nullptr, topic.c_str(), 1) };
    if (result != MOSQ_ERR_SUCCESS) {
        switch (result) {
        case MOSQ_ERR_INVAL:
            Log::error()<<"Could not subscribe to topic '" + topic + "': invalid parameters";
            break;
        case MOSQ_ERR_NOMEM:
            Log::error()<<"Could not subscribe to topic '" + topic + "': memory exceeded";
            break;
        case MOSQ_ERR_NO_CONN:
            Log::error()<<"Could not subscribe to topic '" + topic + "': Not connected";
            break;
        case MOSQ_ERR_MALFORMED_UTF8:
            Log::error()<<"Could not subscribe to topic '" + topic + "': malformed utf8";
            break;
        case MOSQ_ERR_OVERSIZE_PACKET:
            Log::error()<<"Could not subscribe to topic '" + topic + "': oversize packet";
            break;
        }

        throw -1;
    }
    m_subscribers[topic] = std::make_unique<Subscriber>(this, topic);
    Log::debug()<<"Starting to subscribe to topic " + topic;
    return *m_subscribers[topic];
}

auto MqttLink::connect(std::size_t n) -> bool
{
    m_tries++;
    set_status(Status::Connecting);
    static constexpr std::size_t max_tries { 5 };

    if ((n > max_tries) || (m_tries > max_tries*2)) {
        set_status(Status::Error);
        Log::error()<<"Giving up trying to connect to MQTT.";
        return false;
    }
    if (mosquitto_username_pw_set(m_mqtt, m_login_data.username.c_str(), m_login_data.password.c_str()) != MOSQ_ERR_SUCCESS) {
        Log::warning()<<"Could not connect to MQTT";
        return false;
    }
    auto result { mosquitto_connect(m_mqtt, m_host.c_str(), m_port, 60) };
    if (result == MOSQ_ERR_SUCCESS) {
        return true;
    }
    std::this_thread::sleep_for( std::chrono::seconds{1} );
    Log::warning()<<"Could not connect to MQTT: " + std::to_string(result);
    return connect(n + 1);
}

auto MqttLink::disconnect() -> bool
{
    if (m_status != Status::Connected) {
        return true;
    }
    auto result { mosquitto_disconnect(m_mqtt) };
    if (result == MOSQ_ERR_SUCCESS) {
        set_status(Status::Disconnected);
        Log::info()<<"Disconnected from MQTT.";
        return true;
    }
    Log::error()<<"Could not disconnect from MQTT: " + std::to_string(result);
    return false;
}

auto MqttLink::reconnect(std::size_t n) -> bool
{
    m_tries++;
    set_status(Status::Disconnected);
    static constexpr std::size_t max_tries { 5 };

    if ((n > max_tries) || (m_tries > max_tries*2)) {
        set_status(Status::Error);
        Log::error()<<"Giving up trying to reconnect to MQTT.";
        return false;
    }

    Log::info()<<"Trying to reconnect to MQTT.";
    auto result { mosquitto_reconnect(m_mqtt) };
    if (result == MOSQ_ERR_SUCCESS) {
        return true;
    }
    std::this_thread::sleep_for( std::chrono::seconds{1} );
    Log::error()<<"Could not reconnect to MQTT: " + std::to_string(result);
    return reconnect(n + 1);
}

void MqttLink::set_status(Status status) {
    m_status = status;
}

auto MqttLink::Publisher::publish(const std::string& content) -> bool
{
    return m_link->publish(m_topic, content);
}


auto MqttLink::Subscriber::has_message() const -> bool
{
    return m_has_message;
}

auto MqttLink::Subscriber::get_message() -> Message
{
    if (!m_has_message) {
        return {};
    }
    std::scoped_lock<std::mutex> lock { m_mutex };
    auto msg { m_messages.front() };
    m_messages.pop();
    m_has_message = !m_messages.empty();
    return msg;
}

void MqttLink::Subscriber::push_message(const Message &message)
{
    std::scoped_lock<std::mutex> lock { m_mutex };
    m_messages.push(message);
    m_has_message = true;
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
