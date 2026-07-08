#include "sinks/mqtt_sink.h"

#include "core/logging/logger.h"
#include "custom_io_operators.h"
#include "data/events/mqtt_log_event.h"
#include "data/events/mqtt_status_event.h"
#include "data/events/ubx_event.h"

#ifdef Q_OS_UNIX
#include <sys/syscall.h>
#include <unistd.h>
#endif
#include <algorithm>
#include <iostream>
#include <mosquitto.h>
#include <sstream>
#include <string>

void logResult(int result, const std::string& topic) {
    if (result != MOSQ_ERR_SUCCESS) {
        switch (result) {
            case MOSQ_ERR_INVAL:
                logWarn("Could not subscribe to topic '" + topic + "': invalid parameters");
                break;
            case MOSQ_ERR_NOMEM:
                logWarn("Could not subscribe to topic '" + topic + "': memory exceeded");
                break;
            case MOSQ_ERR_NO_CONN:
                logWarn("Could not subscribe to topic '" + topic + "': Not connected");
                break;
            case MOSQ_ERR_MALFORMED_UTF8:
                logWarn("Could not subscribe to topic '" + topic + "': malformed utf8");
                break;
            default:
                logWarn("Could not subscribe to topic '" + topic + "': other reason");
                break;
        }
        return;
    }
    logInfo("Subscribed to topic '" + topic + "'.");
}

MqttSink::MqttSink(EventBus& bus, const std::string& station_id)
    : bus_{bus}, m_station_id{station_id} {
}

MqttSink::~MqttSink() {
    cleanup();
}

void MqttSink::shutdown() {
    cleanup();
}

void MqttSink::handle(const UbxTimeMarkStruct& tm) {
    // output is: rising falling timeAcc valid timeBase utcAvailable
    std::stringstream sstr;
    sstr << tm.rising << tm.falling << tm.accuracy_ns << " " << tm.evtCounter << " "
         << static_cast<short>(tm.valid) << " " << static_cast<short>(tm.timeBase) << " "
         << static_cast<short>(tm.utcAvailable);
    publish(MuonPi::Config::MQTT::data_topic, sstr.str());
}

void MqttSink::handle(const MqttLogEvent& event) {
    publish(MuonPi::Config::MQTT::log_topic, event.msg);
}

void wrapper_callback_connected(mosquitto* /*mqtt*/, void* object, int result) {
    logDebug("Wrapper callback connected");
    reinterpret_cast<MqttSink*>(object)->callback_connected(result);
}

void wrapper_callback_disconnected(mosquitto* /*mqtt*/, void* object, int result) {
    logDebug("Wrapper callback disconnected");
    reinterpret_cast<MqttSink*>(object)->callback_disconnected(result);
}

void wrapper_callback_message(mosquitto* /*mqtt*/, void* object, const mosquitto_message* message) {
    logDebug("Wrapper callback message");
    reinterpret_cast<MqttSink*>(object)->callback_message(message);
}

void MqttSink::callback_connected(int result) {
    if (result == 1) {
        setStatus(MqttStatusEvent::Status::Error);
        const std::string msg{"MQTT connection failed: Wrong protocol version"};
        logWarn(msg);
        bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Error, msg});
    } else if (result == 2) {
        setStatus(MqttStatusEvent::Status::Error);
        const std::string msg{"MQTT connection failed: Credentials rejected"};
        logWarn(msg);
        bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Error, msg});
    } else if (result == 3) {
        setStatus(MqttStatusEvent::Status::Error);
        const std::string msg{"MQTT connection failed: Broker unavailable"};
        logWarn(msg);
        bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Error, msg});
    } else if (result > 3) {
        setStatus(MqttStatusEvent::Status::Error);
        const std::string msg{"MQTT connection failed: Other reason"};
        logWarn(msg);
        bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Error, msg});
    } else if (result == 0) {
        setStatus(MqttStatusEvent::Status::Connected);
        const std::string msg{"Connected to MQTT."};
        logInfo(msg);
        bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Connected, msg});
        m_tries = 0;
        return;
    }
    if (m_tries < s_max_tries) {
        m_tries++;
    } else {
        setStatus(MqttStatusEvent::Status::Error);
        const std::string msg{"Too many retries."};
        logWarn(msg);
        bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Error, msg});
    }
    std::string msg{"Connecting to MQTT..."};
    setStatus(MqttStatusEvent::Status::Connecting);
    logInfo(msg);
    bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Connecting, msg});
    logInfo("Tried to connect " + std::to_string(m_tries) + " times. Next try after " +
            std::to_string(first_retry_period) + " " + std::to_string(m_tries) + "s.");
}

void MqttSink::callback_disconnected(int result) {
    if (result != 0) {
        if (connected()) {
            setStatus(MqttStatusEvent::Status::Error);
            std::string msg{"MQTT disconnected unexpectedly: " + std::to_string(result)};
            logWarn(msg);
            bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Error, msg});
        }
    } else {
        setStatus(MqttStatusEvent::Status::Disconnected);
        bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Disconnected, "MQTT disconnected."});
    }
}

void MqttSink::callback_message(const mosquitto_message* message) {
    // emit receivedMessage(message->topic, reinterpret_cast<char *>(message->payload));
}

void MqttSink::setStatus(MqttStatusEvent::Status status) {
    logDebug("set_status " + std::to_string(static_cast<int>(status)));
    m_status.store(status);
}

bool MqttSink::isInhibited() {
    return (m_status.load() == MqttStatusEvent::Status::Inhibited);
}

void MqttSink::setInhibited(bool inhibited) {
    if (inhibited) {
        if (m_status.load() == MqttStatusEvent::Status::Connected) {
            setStatus(MqttStatusEvent::Status::Inhibited);
            bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Inhibited});
        }
    } else {
        setStatus(MqttStatusEvent::Status::Invalid);
        bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Invalid});
    }
}

void MqttSink::start(const std::string& username, const std::string& password) {
    m_username = username;
    m_password = password;

    m_client_id = username + "_" + m_station_id;

    initialise(m_client_id);

    mqttConnect();
}

void MqttSink::mqttConnect() {
    if (connected()) {
        return;
    }
    logDebug("Trying to connect to MQTT.");

    if (m_mqtt == nullptr) {
        logDebug("MQTT not initialized on connect called. Initializing...");
        initialise(m_client_id);
    }

    setStatus(MqttStatusEvent::Status::Connecting);
    bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Connecting});

    auto result{mosquitto_username_pw_set(m_mqtt, m_username.c_str(), m_password.c_str())};
    if (result != MOSQ_ERR_SUCCESS) {
        setStatus(MqttStatusEvent::Status::Error);
        logWarn("Error setting username and password: " + std::string(mosquitto_strerror(result)));
        return;
    }
    result = mosquitto_connect_async(m_mqtt, MuonPi::Config::MQTT::host, MuonPi::Config::MQTT::port,
                                     MuonPi::Config::MQTT::keepalive_interval.count());
    if (result != MOSQ_ERR_SUCCESS) {
        setStatus(MqttStatusEvent::Status::Error);
        logDebug("Error on called mosquitto_connect_async");
        return;
    }
}

void MqttSink::mqttDisconnect() {
    if (m_mqtt != nullptr) {
        if (m_status.load() == MqttStatusEvent::Status::Connected) {
            setStatus(MqttStatusEvent::Status::Disconnecting);
            bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Disconnecting});
            auto result{mosquitto_disconnect(m_mqtt)};
            if (result == MOSQ_ERR_SUCCESS) {
                setStatus(MqttStatusEvent::Status::Disconnected);
                bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Disconnected});
            } else {
                std::string msg{"Could not disconnect from Mqtt: "};
                logDebug("Could not disconnect from Mqtt: " +
                         std::string{mosquitto_strerror(result)});
                setStatus(MqttStatusEvent::Status::Invalid);
                bus_.publish(MqttStatusEvent{MqttStatusEvent::Status::Invalid});
            }
        }
    }
}

auto MqttSink::connected() -> bool {
    return (m_mqtt != nullptr) && (m_status.load() == MqttStatusEvent::Status::Connected);
}

void MqttSink::initialise(const std::string& client_id) {
    if (m_mqtt != nullptr) {
        cleanup();
    }

    mosquitto_lib_init();

    m_mqtt = mosquitto_new(client_id.c_str(), true, this);

    // exponential backoff set to true. Example retry periods: 2, 4, 8, 16, 32 ...
    mosquitto_reconnect_delay_set(m_mqtt, first_retry_period, max_retry_period, true);

    mosquitto_connect_callback_set(m_mqtt, wrapper_callback_connected);
    mosquitto_disconnect_callback_set(m_mqtt, wrapper_callback_disconnected);
    mosquitto_message_callback_set(m_mqtt, wrapper_callback_message);

    mosquitto_loop_start(m_mqtt);
}

void MqttSink::cleanup() {
    if (connected()) {
        mqttDisconnect();
    }
    if (m_mqtt != nullptr) {
        mosquitto_loop_stop(m_mqtt, true);

        mosquitto_destroy(m_mqtt);
        m_mqtt = nullptr;
        mosquitto_lib_cleanup();
    }
}

void MqttSink::subscribe(const std::string& topic) {
    if (!connected()) {
        return;
    }

    m_topics.emplace_back(topic);

    auto result{mosquitto_subscribe(m_mqtt, nullptr, topic.c_str(), 1)};

    logResult(result, topic);
}

void MqttSink::unsubscribe(const std::string& topic) {
    if (!connected()) {
        return;
    }

    auto pos{std::find(std::begin(m_topics), std::end(m_topics), topic)};
    if (pos != std::end(m_topics)) {
        m_topics.erase(pos);
    }

    auto result{mosquitto_unsubscribe(m_mqtt, nullptr, topic.c_str())};

    logResult(result, topic);
}

auto MqttSink::publish(const std::string& topic, const std::string& content) -> bool {
    if (!connected()) {
        return false;
    }
    std::string usertopic{topic};
    usertopic += m_username + "/" + m_station_id;

    auto result{mosquitto_publish(m_mqtt, nullptr, usertopic.c_str(),
                                  static_cast<int>(content.size()),
                                  reinterpret_cast<const void*>(content.c_str()), 1, false)};

    if (result == MOSQ_ERR_SUCCESS) {
        m_publish_error_count = 0;
        return true;
    }
    const auto error_count = ++m_publish_error_count;
    if (error_count < s_max_publish_errors) {
        logWarn("Couldn't publish mqtt message for topic '" + usertopic +
                "': " + std::string{mosquitto_strerror(result)});
    } else if (error_count == s_max_publish_errors) {
        logWarn("Couldn't publish mqtt message for topic '" + usertopic + "' (message repeated " +
                std::to_string(s_max_publish_errors) +
                " times): " + std::string{mosquitto_strerror(result)});
    }
    return false;
}

void MqttSink::requestConnectionStatus() {
    const auto status = m_status.load();
    std::string msg{"connection status = " + std::to_string(static_cast<int>(status))};
    logDebug(msg);
    bus_.publish(MqttStatusEvent{status, msg});
}

auto MqttSink::connectionStatus() const -> MqttStatusEvent::Status {
    return m_status.load();
}
