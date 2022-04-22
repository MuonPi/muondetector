#include "mqtthandler.h"


#include <QDebug>
#include <QTimer>
#include <QThread>
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

namespace MuonPi {

void wrapper_callback_connected(mosquitto* /*mqtt*/, void* object, int result)
{
    reinterpret_cast<MqttHandler*>(object)->callback_connected(result);
}

void wrapper_callback_disconnected(mosquitto* /*mqtt*/, void* object, int result)
{
    reinterpret_cast<MqttHandler*>(object)->callback_disconnected(result);
}

void wrapper_callback_message(mosquitto* /*mqtt*/, void* object, const mosquitto_message* message)
{
    reinterpret_cast<MqttHandler*>(object)->callback_message(message);
}

void MqttHandler::callback_connected(int result)
{
//    qDebug() << "callback_connected() called, result code ="<<result;
    if (result == 1) {
        qWarning() << "MQTT connection failed: Wrong protocol version";
    } else if (result == 2) {
        qWarning() << "MQTT connection failed: Credentials rejected";
    } else if (result == 3) {
        qWarning() << "MQTT connection failed: Broker unavailable";
    } else if (result > 3) {
        qWarning() << "MQTT connection failed: Other reason";
    } else if (result == 0) {
        qInfo() << "Connected to MQTT.";
        emit connection_status(Status::Connected);
        m_tries = 0;
        return;
    }
    emit connection_status(Status::Error);
}

void MqttHandler::callback_disconnected(int result)
{
    if (result != 0) {
        qWarning() << "MQTT disconnected unexpectedly:" + QString::number(result);
        emit connection_status(Status::Error);
    } else {
        emit connection_status(Status::Disconnected);
    }
}

void MqttHandler::callback_message(const mosquitto_message* message)
{
    emit receivedMessage(message->topic, reinterpret_cast<char*>(message->payload));
}

void MqttHandler::set_status(Status status)
{
    if (status == Status::Connected) {
        m_tries = 0;
    }
    if (status != m_status) {
        m_status = status;        
        onTimer();
    }
}

MqttHandler::MqttHandler(const QString& station_id, const int verbosity)
    : QObject(nullptr)
    , m_station_id { station_id.toStdString() }
    , m_verbose { verbosity }
{
    qRegisterMetaType<Status>("Status");

    m_reconnect_timer.setSingleShot(false);
    m_reconnect_timer.setInterval(3000);
//    connect(&m_reconnect_timer, &QTimer::timeout, this, [this](){mqttConnect();});
/*
    if (!connect(&m_reconnect_timer, &QTimer::timeout, this, &MqttHandler::onTimer)) {
        qDebug() << "failed connection QTimer::timeout";
    }
    if (!connect(this, &MqttHandler::request_timer_start, this, &MqttHandler::timer_start)) {
        qDebug() << "failed connecting MqttHandler::request_timer_start";
    }
    if (!connect(this, &MqttHandler::request_timer_stop, this, &MqttHandler::timer_stop)) {
        qDebug() << "failed connecting MqttHandler::request_timer_stop";
    }
    */
    if (!connect(this, &MqttHandler::connection_status, this, &MqttHandler::set_status)) {
        qDebug() << "failed connecting MqttHandler::set_status";
    }
    if (!connect(this, &MqttHandler::mqttConnect, this, &MqttHandler::onMqttConnect)) {
        qDebug() << "failed connecting MqttHandler::mqttConnect";
    }
    if (!connect(this, &MqttHandler::mqttDisconnect, this, &MqttHandler::onMqttDisconnect)) {
        qDebug() << "failed connecting MqttHandler::mqttDisconnect";
    }
    //m_reconnect_timer.start();
}

MqttHandler::~MqttHandler()
{
    mqttDisconnect();
    cleanup();
}

void MqttHandler::onTimer()
{
    if (m_status == Status::Error) {
        //emit mqttDisconnect();
        m_tries++;
        qDebug() << "Tried: "<<m_tries;
        if (m_tries > s_max_tries) {
            emit giving_up();
            exit(0);
        }
        qWarning() << "Could not connect to MQTT. Trying again in " + QString::number(std::chrono::duration_cast<std::chrono::seconds>(Config::MQTT::retry_period).count() * (1<<(m_tries-1))) + "s";
        emit connection_status(Status::Invalid);
/*
        QTimer::singleShot(std::chrono::seconds(Config::MQTT::retry_period.count() * (1<<(m_tries-1))), [this]() {
//            emit mqttDisconnect();
            emit mqttConnect();
        });
        return;
*/
        std::this_thread::sleep_for(std::chrono::seconds(Config::MQTT::retry_period.count() * (1<<(m_tries-1))) );
        emit mqttDisconnect();
        emit mqttConnect();
    }
}

void MqttHandler::timer_restart(int timeout)
{
    m_reconnect_timer.stop();
    m_reconnect_timer.start(timeout);
}

void MqttHandler::timer_start(int timeout)
{
    qDebug()<<"starting timer with"<<timeout/1000<<"s";
    m_reconnect_timer.start(timeout);
}

void MqttHandler::timer_stop()
{
    qDebug() << "stopping timer";
    m_reconnect_timer.stop();
}

void MqttHandler::start(const QString& username, const QString& password){
    m_username = username.toStdString();
    m_password = password.toStdString();

    m_data_topic = Config::MQTT::data_topic + m_username + "/" + m_station_id;
    m_log_topic =  Config::MQTT::log_topic + m_username + "/" + m_station_id;

    CryptoPP::SHA1 sha1;
    std::string source = username.toStdString()+m_station_id;  //This will be randomly generated somehow
    m_client_id = "";
    CryptoPP::StringSource{source, true, new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(new CryptoPP::StringSink(m_client_id)))};

    initialise(m_client_id);

    emit mqttConnect();
}

void MqttHandler::onMqttConnect(){
    if (connected()) {
        qDebug() << "already connected";
        return;
    }
    qDebug() << "Trying to connect to MQTT.";

    if (m_mqtt == nullptr) {
        initialise(m_client_id);
    }

    emit connection_status(Status::Connecting);

    auto result { mosquitto_username_pw_set(m_mqtt, m_username.c_str(), m_password.c_str()) };
    if (result != MOSQ_ERR_SUCCESS) {
        qWarning() << "Error setting username and password:" + QString{ strerror(result) };
        return;
    }
    result = mosquitto_connect(m_mqtt, Config::MQTT::host, Config::MQTT::port, Config::MQTT::keepalive_interval.count());
    if (result == MOSQ_ERR_SUCCESS) {
        return;
    }
    qDebug() << "Error establishing connection";
    
    //emit request_timer_start(std::chrono::duration_cast<std::chrono::milliseconds>(Config::MQTT::retry_period).count() * (1<<(m_tries)) + 2000UL);
//emit request_timer_start(Config::MQTT::timeout * m_tries);
}

void MqttHandler::onMqttDisconnect(){
    emit connection_status(Status::Disconnecting);
    auto result { mosquitto_disconnect(m_mqtt) };
    if (result == MOSQ_ERR_SUCCESS) {
        return;
    }
    qDebug() << "Could not disconnect from Mqtt:" + QString{ strerror(result) };
}

auto MqttHandler::connected() -> bool{
    return (m_mqtt != nullptr) && (m_status == Status::Connected);
}

void MqttHandler::initialise(const std::string& client_id)
{
    if (m_mqtt != nullptr) {
        cleanup();
    }

    mosquitto_lib_init();

    m_mqtt = mosquitto_new(client_id.c_str(), true, this);

    mosquitto_connect_callback_set(m_mqtt, wrapper_callback_connected);
    mosquitto_disconnect_callback_set(m_mqtt, wrapper_callback_disconnected);
    mosquitto_message_callback_set(m_mqtt, wrapper_callback_message);

    mosquitto_loop_start(m_mqtt);
}

void MqttHandler::cleanup() {
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

void MqttHandler::subscribe(const QString& topic)
{
    if (!connected()) {
        return;
    }

    m_topics.emplace_back(topic.toStdString());

    auto result { mosquitto_subscribe(m_mqtt, nullptr, topic.toStdString().c_str(), 1) };

    if (result != MOSQ_ERR_SUCCESS) {
        switch (result) {
        case MOSQ_ERR_INVAL:
            qWarning() << "Could not subscribe to topic '" + topic + "': invalid parameters";
            break;
        case MOSQ_ERR_NOMEM:
            qWarning() << "Could not subscribe to topic '" + topic + "': memory exceeded";
            break;
        case MOSQ_ERR_NO_CONN:
            qWarning() << "Could not subscribe to topic '" + topic + "': Not connected";
            break;
        case MOSQ_ERR_MALFORMED_UTF8:
            qWarning() << "Could not subscribe to topic '" + topic + "': malformed utf8";
            break;
        default:
            qWarning() << "Could not subscribe to topic '" + topic + "': other reason";
            break;
        }
        return;
    }
    qInfo() << "Subscribed to topic '" + topic + "'.";
}

void MqttHandler::unsubscribe(const QString& topic)
{
    if (!connected()) {
        return;
    }

    auto pos { std::find(std::begin(m_topics), std::end(m_topics), topic.toStdString()) };
    if (pos != std::end(m_topics)) {
        m_topics.erase(pos);
    }

    auto result { mosquitto_unsubscribe(m_mqtt, nullptr, topic.toStdString().c_str()) };

    if (result != MOSQ_ERR_SUCCESS) {
        switch (result) {
        case MOSQ_ERR_INVAL:
            qWarning() << "Could not unsubscribe from topic '" + topic + "': invalid parameters";
            break;
        case MOSQ_ERR_NOMEM:
            qWarning() << "Could not unsubscribe from topic '" + topic + "': memory exceeded";
            break;
        case MOSQ_ERR_NO_CONN:
            qWarning() << "Could not unsubscribe from topic '" + topic + "': Not connected";
            break;
        case MOSQ_ERR_MALFORMED_UTF8:
            qWarning() << "Could not unsubscribe from topic '" + topic + "': malformed utf8";
            break;
        default:
            qWarning() << "Could not unsubscribe from topic '" + topic + "': other reason";
            break;
        }
        return;
    }
    qInfo() << "Unsubscribed from topic '" + topic + "'.";
}

void MqttHandler::sendData(const QString &message){
    if (!publish(m_data_topic, message.toStdString())) {
        m_data_error_count++;
        if ( m_data_error_count < s_max_publish_errors ) {
            qWarning() << "Couldn't publish data";
        } else if ( m_data_error_count == s_max_publish_errors ) {
            qWarning() << "Couldn't publish data (message repeated" << s_max_publish_errors << "times)";
        }
        return;
    }
    m_data_error_count = 0;
}

void MqttHandler::sendLog(const QString &message){
    if (!publish(m_log_topic, message.toStdString())) {
        m_log_error_count++;
        if ( m_log_error_count < s_max_publish_errors ) {
            qWarning() << "Couldn't publish log";
        } else if ( m_log_error_count == s_max_publish_errors ) {
            qWarning() << "Couldn't publish log (message repeated" << s_max_publish_errors << "times)";
        }
        return;
    }
    m_log_error_count = 0;
}

auto MqttHandler::publish(const std::string& topic, const std::string& content) -> bool {
    if (!connected()) {
        return false;
    }
    auto result { mosquitto_publish(m_mqtt, nullptr, topic.c_str(), static_cast<int>(content.size()), reinterpret_cast<const void*>(content.c_str()), 1, false) };

    if (result == MOSQ_ERR_SUCCESS) {
        return true;
    }
    qWarning() << "Couldn't publish mqtt message: " + QString{strerror(result)};
    return false;
}

void MqttHandler::onRequestConnectionStatus(){
    qDebug() << "connection status = "<<QString::number(static_cast<int>(m_status));
    //emit mqttConnectionStatus(m_status == Status::Connected);
    emit connection_status(m_status);
}
} // namespace MuonPi
