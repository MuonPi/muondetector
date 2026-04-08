#include "mqtthandler.h"

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <sys/syscall.h>
#endif
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <string>
#include <algorithm>

#include <iostream>

namespace MuonPi
{

    void wrapper_callback_connected(mosquitto * /*mqtt*/, void *object, int result)
    {
        reinterpret_cast<MqttHandler *>(object)->callback_connected(result);
    }

    void wrapper_callback_disconnected(mosquitto * /*mqtt*/, void *object, int result)
    {
        reinterpret_cast<MqttHandler *>(object)->callback_disconnected(result);
    }

    void wrapper_callback_message(mosquitto * /*mqtt*/, void *object, const mosquitto_message *message)
    {
        reinterpret_cast<MqttHandler *>(object)->callback_message(message);
    }

    void MqttHandler::callback_connected(int result)
    {
        if (result == 1)
        {
            std::cerr << "MQTT connection failed: Wrong protocol version";
            // emit connection_status(Status::Error);
        }
        else if (result == 2)
        {
            std::cerr << "MQTT connection failed: Credentials rejected";
            // emit connection_status(Status::Error);
        }
        else if (result == 3)
        {
            std::cerr << "MQTT connection failed: Broker unavailable";
            // emit connection_status(Status::Error);
        }
        else if (result > 3)
        {
            std::cerr << "MQTT connection failed: Other reason";
            // emit connection_status(Status::Error);
        }
        else if (result == 0)
        {
            std::cout << "Connected to MQTT.";
            // emit connection_status(Status::Connected);
            m_tries = 0;
            return;
        }
        if (m_tries < s_max_tries)
        {
            m_tries++;
        }
        // emit connection_status(Status::Connecting);
        std::cout << "Tried to connect " << m_tries << " times. Next try after " << (first_retry_period << m_tries) << "s.";
    }

    void MqttHandler::callback_disconnected(int result)
    {
        if (result != 0)
        {
            if (connected())
            {
                std::cerr << "MQTT disconnected unexpectedly:" + std::to_string(result);
                // emit connection_status(Status::Error);
            }
        }
        else
        {
            // emit connection_status(Status::Disconnected);
        }
    }

    void MqttHandler::callback_message(const mosquitto_message *message)
    {
        // emit receivedMessage(message->topic, reinterpret_cast<char *>(message->payload));
    }

    void MqttHandler::set_status(Status status)
    {
        std::cout << "set_status" << static_cast<int>(status);
        m_status = status;
    }

    bool MqttHandler::isInhibited()
    {
        return (m_status == Status::Inhibited);
    }

    void MqttHandler::setInhibited(bool inhibited)
    {
        if (inhibited)
        {
            if (m_status == Status::Connected)
            {
                // emit connection_status(Status::Inhibited);
            }
        }
        else
        {
            // emit connection_status(Status::Invalid);
        }
    }

    MqttHandler::MqttHandler(const std::string &station_id, const int verbosity)
        : m_station_id{station_id}, m_verbose{verbosity}
    {
        // qRegisterMetaType<Status>("Status");

        // if (!connect(this, &MqttHandler::connection_status, this, &MqttHandler::set_status))
        // {
        //     std::cout << "failed connecting MqttHandler::set_status";
        // }
        // if (!connect(this, &MqttHandler::mqttConnect, this, &MqttHandler::onMqttConnect))
        // {
        //     std::cout << "failed connecting MqttHandler::mqttConnect";
        // }
        // if (!connect(this, &MqttHandler::mqttDisconnect, this, &MqttHandler::onMqttDisconnect))
        // {
        //     std::cout << "failed connecting MqttHandler::mqttDisconnect";
        // }
    }

    MqttHandler::~MqttHandler()
    {
        // mqttDisconnect();
        cleanup();
    }

    void MqttHandler::start(const std::string &username, const std::string &password)
    {
        m_username = username;
        m_password = password;

        CryptoPP::SHA1 sha1;
        std::string source = username + m_station_id; // This will be randomly generated somehow
        m_client_id = "";
        CryptoPP::StringSource{source, true, new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(new CryptoPP::StringSink(m_client_id)))};

        initialise(m_client_id);

        // emit mqttConnect();
    }

    // void MqttHandler::onMqttConnect()
    // {
    //     if (connected())
    //     {
    //         return;
    //     }
    //     std::cout << "Trying to connect to MQTT.";

    //     if (m_mqtt == nullptr)
    //     {
    //         std::cout << "MQTT not initialized on connect called. Initializing...";
    //         initialise(m_client_id);
    //     }

    //     emit connection_status(Status::Connecting);

    //     auto result{mosquitto_username_pw_set(m_mqtt, m_username.c_str(), m_password.c_str())};
    //     if (result != MOSQ_ERR_SUCCESS)
    //     {
    //         std::cerr << "Error setting username and password:" + std::string{strerror(result)};
    //         return;
    //     }
    //     result = mosquitto_connect_async(m_mqtt, Config::MQTT::host, Config::MQTT::port, Config::MQTT::keepalive_interval.count());
    //     if (result != MOSQ_ERR_SUCCESS)
    //     {
    //         std::cout << "Error on called mosquitto_connect_async";
    //         return;
    //     }
    // }

    // void MqttHandler::onMqttDisconnect()
    // {
    //     if (m_mqtt != nullptr)
    //     {
    //         if (m_status == Status::Connected)
    //         {
    //             emit connection_status(Status::Disconnecting);
    //             auto result{mosquitto_disconnect(m_mqtt)};
    //             if (result == MOSQ_ERR_SUCCESS)
    //             {
    //                 connection_status(Status::Disconnected);
    //             }
    //             else
    //             {
    //                 std::cout << "Could not disconnect from Mqtt: " + std::string{strerror(result)};
    //                 connection_status(Status::Invalid);
    //             }
    //         }
    //     }
    // }

    auto MqttHandler::connected() -> bool
    {
        return (m_mqtt != nullptr) && (m_status == Status::Connected);
    }

    void MqttHandler::initialise(const std::string &client_id)
    {
        if (m_mqtt != nullptr)
        {
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

    void MqttHandler::cleanup()
    {
        if (connected())
        {
            // emit mqttDisconnect();
        }
        if (m_mqtt != nullptr)
        {
            mosquitto_loop_stop(m_mqtt, true);

            mosquitto_destroy(m_mqtt);
            m_mqtt = nullptr;
            mosquitto_lib_cleanup();
        }
    }

    void MqttHandler::subscribe(const std::string &topic)
    {
        if (!connected())
        {
            return;
        }

        m_topics.emplace_back(topic);

        auto result{mosquitto_subscribe(m_mqtt, nullptr, topic.c_str(), 1)};

        if (result != MOSQ_ERR_SUCCESS)
        {
            switch (result)
            {
            case MOSQ_ERR_INVAL:
                std::cerr << "Could not subscribe to topic '" + topic + "': invalid parameters";
                break;
            case MOSQ_ERR_NOMEM:
                std::cerr << "Could not subscribe to topic '" + topic + "': memory exceeded";
                break;
            case MOSQ_ERR_NO_CONN:
                std::cerr << "Could not subscribe to topic '" + topic + "': Not connected";
                break;
            case MOSQ_ERR_MALFORMED_UTF8:
                std::cerr << "Could not subscribe to topic '" + topic + "': malformed utf8";
                break;
            default:
                std::cerr << "Could not subscribe to topic '" + topic + "': other reason";
                break;
            }
            return;
        }
        std::cout << "Subscribed to topic '" + topic + "'.";
    }

    void MqttHandler::unsubscribe(const std::string &topic)
    {
        if (!connected())
        {
            return;
        }

        auto pos{std::find(std::begin(m_topics), std::end(m_topics), topic)};
        if (pos != std::end(m_topics))
        {
            m_topics.erase(pos);
        }

        auto result{mosquitto_unsubscribe(m_mqtt, nullptr, topic.c_str())};

        if (result != MOSQ_ERR_SUCCESS)
        {
            switch (result)
            {
            case MOSQ_ERR_INVAL:
                std::cerr << "Could not unsubscribe from topic '" + topic + "': invalid parameters";
                break;
            case MOSQ_ERR_NOMEM:
                std::cerr << "Could not unsubscribe from topic '" + topic + "': memory exceeded";
                break;
            case MOSQ_ERR_NO_CONN:
                std::cerr << "Could not unsubscribe from topic '" + topic + "': Not connected";
                break;
            case MOSQ_ERR_MALFORMED_UTF8:
                std::cerr << "Could not unsubscribe from topic '" + topic + "': malformed utf8";
                break;
            default:
                std::cerr << "Could not unsubscribe from topic '" + topic + "': other reason";
                break;
            }
            return;
        }
        std::cout << "Unsubscribed from topic '" + topic + "'.";
    }

    // void MqttHandler::publish(const std::string &topic, const std::string &content)
    // {
    //     if (!connected())
    //     {
    //         return;
    //     }
    //     std::string usertopic{topic};
    //     usertopic += m_username + "/" + m_station_id;
    //     if (!publish(usertopic, content))
    //     {
    //         m_publish_error_count++;
    //         if (m_publish_error_count < s_max_publish_errors)
    //         {
    //             std::cerr << "Couldn't publish message for topic" << topic;
    //         }
    //         else if (m_publish_error_count == s_max_publish_errors)
    //         {
    //             std::cerr << "Couldn't publish message for topic" << topic << "(message repeated" << s_max_publish_errors << "times)";
    //         }
    //         return;
    //     }
    //     m_publish_error_count = 0;
    // }

    auto MqttHandler::publish(const std::string &topic, const std::string &content) -> bool
    {
        if (!connected())
        {
            return false;
        }
        auto result{mosquitto_publish(m_mqtt, nullptr, topic.c_str(), static_cast<int>(content.size()), reinterpret_cast<const void *>(content.c_str()), 1, false)};

        if (result == MOSQ_ERR_SUCCESS)
        {
            return true;
        }
        std::cerr << "Couldn't publish mqtt message: " + std::string{strerror(result)};
        return false;
    }

    void MqttHandler::requestConnectionStatus()
    {
        std::cout << "connection status = " << std::to_string(static_cast<int>(m_status));
        // emit connection_status(m_status);
    }
} // namespace MuonPi
