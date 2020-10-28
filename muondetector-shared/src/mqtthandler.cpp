#include "mqtthandler.h"

#include <QDebug>
#include <QTimer>
#include <QThread>
#include <unistd.h>
#include <sys/syscall.h>
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <string>

#include <iostream>

namespace MuonPi {


void callback::connection_lost(const std::string& cause) {
    std::cout << "\nConnection lost" << std::endl;
    if (!cause.empty()) {
        std::cout << "\tcause: " << cause << std::endl;
    }
}

void callback::delivery_complete(mqtt::delivery_token_ptr tok) {
    std::cout << "\tDelivery complete for token: "
        << (tok ? tok->get_message_id() : -1) << std::endl;
}

void callback::message_arrived(mqtt::const_message_ptr /*message*/)
{
}

void action_listener::on_failure(const mqtt::token& tok) {
    std::cout << "\tListener failure for token: "
        << tok.get_message_id() << std::endl;
}

void action_listener::on_success(const mqtt::token& tok) {
    std::cout << "\tListener success for token: "
        << tok.get_message_id() << std::endl;
}

void delivery_action_listener::on_failure(const mqtt::token& tok) {
    action_listener::on_failure(tok);
    m_done = true;
}

void delivery_action_listener::on_success(const mqtt::token& tok) {
    action_listener::on_success(tok);
    m_done = true;
}

MqttHandler::MqttHandler(const QString& station_ID, const int verbosity)
    : m_stationID { station_ID.toStdString() }
    , m_verbose { verbosity }
{
}

void MqttHandler::start(const QString& username, const QString& password){
    //qInfo() << this->thread()->objectName() << " thread id (pid): " << syscall(SYS_gettid);
    m_username = username.toStdString();
    m_password = password.toStdString();
    CryptoPP::SHA1 sha1;
    std::string source = username.toStdString()+m_stationID;  //This will be randomly generated somehow
    m_clientID = "";
    CryptoPP::StringSource{source, true, new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(new CryptoPP::StringSink(m_clientID)))};
    m_reconnectTimer = new QTimer();
    m_reconnectTimer->setInterval(Config::MQTT::timeout);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this](){mqttConnect();});
    mqttStartConnection();
}

void MqttHandler::mqttStartConnection(){
    m_mqttClient = new mqtt::async_client(std::string{ Config::MQTT::server }, m_clientID);
    m_conopts = new mqtt::connect_options();
    m_conopts->set_user_name(m_username);
    m_conopts->set_password(m_password);
    m_conopts->set_keep_alive_interval(Config::MQTT::keepalive_interval);
    //willmsg = new mqtt::message("muonpi/data/", "Last will and testament.", 1, true);
    //will = new mqtt::will_options(*willmsg);
    //conopts->set_will(*will)
    mqttConnect();
}

void MqttHandler::mqttConnect(){
    try {
        m_mqttClient->connect(*m_conopts)->wait();
        // TODO add callback
        m_data_topic = new mqtt::topic(*m_mqttClient, "muonpi/data/"+m_username+"/"+m_stationID,Config::MQTT::qos);
        m_config_topic = new mqtt::topic(*m_mqttClient, "muonpi/config/"+m_username+"/"+m_stationID,Config::MQTT::qos);
        m_config_topic->subscribe();
        m_log_topic = new mqtt::topic(*m_mqttClient, "muonpi/log/"+m_username+"/"+m_stationID,Config::MQTT::qos);
        emit mqttConnectionStatus(true);
        m_mqttConnectionStatus = true;
//        qInfo() << "MQTT connected";
    }
    catch (const mqtt::exception& exc) {
        emit mqttConnectionStatus(false);
    qWarning() << QString::fromStdString(exc.what());
        m_reconnectTimer->start();
    }
}

void MqttHandler::mqttDisconnect(){
    // Disconnect
    if (m_mqttClient == nullptr){
        return;
    }
    try {
        mqtt::token_ptr conntok = m_mqttClient->disconnect();
        conntok->wait();
        emit mqttConnectionStatus(false);
        m_mqttConnectionStatus = false;
    }
    catch (const mqtt::exception& exc) {
        qWarning() << QString::fromStdString(exc.what());
    }
}

void MqttHandler::sendData(const QString &message){
    if (m_data_topic == nullptr){
        return;
    }
    try {
        mqtt::token_ptr pubtok = m_data_topic->publish(message.toStdString());
        bool ok=pubtok->wait_for(Config::MQTT::timeout);
        if (ok)
        {
            if (!m_mqttConnectionStatus) {
                //qDebug() << "MQTT publish succeeded";
                emit mqttConnectionStatus(true);
                m_mqttConnectionStatus = true;
            }
        } else {
            //qDebug() << "MQTT publish timeout";
            if (m_mqttConnectionStatus) {
                emit mqttConnectionStatus(false);
                m_mqttConnectionStatus = false;
            }
        }
    }
    catch (const mqtt::exception& exc) {
        qDebug() << QString::fromStdString(exc.what());
        qDebug() << "MQTT publish failed";
        qDebug() << "trying to reconnect...";
        try {
            if (m_mqttClient!=nullptr) {
                mqtt::token_ptr conntok = m_mqttClient->reconnect();
                bool ok=conntok->wait_for(Config::MQTT::timeout);
                if (ok)
                {
                    //qDebug() << "MQTT reconnected";
                    emit mqttConnectionStatus(true);
                    m_mqttConnectionStatus = true;
                } else {

                    qDebug() << "MQTT reconnect timeout";
                    emit mqttConnectionStatus(false);
                    m_mqttConnectionStatus = false;
                }
            }
        } catch (const mqtt::exception& exc) {
            qDebug() << "MQTT reconnect failed";
            qDebug() << QString::fromStdString(exc.what());
            emit mqttConnectionStatus(false);
            m_mqttConnectionStatus = false;
        }
    }
}

void MqttHandler::sendLog(const QString &message){
    if (m_log_topic == nullptr){
        return;
    }
    try {
        m_log_topic->publish(message.toStdString());
    }
    catch (const mqtt::exception& exc) {
    qDebug() << QString::fromStdString(exc.what());
    emit mqttConnectionStatus(false);
    m_mqttConnectionStatus = false;
    }
}

void MqttHandler::onRequestConnectionStatus(){
    emit mqttConnectionStatus(m_mqttConnectionStatus);
}
} // namespace MuonPi
