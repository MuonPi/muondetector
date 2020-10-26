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


void callback::connection_lost(const std::string& cause) {
    std::cout << "\nConnection lost" << std::endl;
    if (!cause.empty())
        std::cout << "\tcause: " << cause << std::endl;
}

void callback::delivery_complete(mqtt::delivery_token_ptr tok) {
    std::cout << "\tDelivery complete for token: "
        << (tok ? tok->get_message_id() : -1) << std::endl;
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

MqttHandler::MqttHandler(QString station_ID, int verbosity){
    m_stationID = station_ID;
    m_verbose=verbosity;
}

void MqttHandler::start(QString username, QString password){
    //qInfo() << this->thread()->objectName() << " thread id (pid): " << syscall(SYS_gettid);
    m_username = username;
    m_password = password;
    CryptoPP::SHA1 sha1;
    std::string source = username.toStdString()+m_stationID.toStdString();  //This will be randomly generated somehow
    m_clientID = "";
    CryptoPP::StringSource(source, true, new CryptoPP::HashFilter(sha1, new CryptoPP::HexEncoder(new CryptoPP::StringSink(m_clientID))));
    m_reconnectTimer = new QTimer();
    m_reconnectTimer->setInterval(m_timeout);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this](){mqttConnect();});
    mqttStartConnection();
}

void MqttHandler::mqttStartConnection(){
    m_mqttClient = new mqtt::async_client(m_mqttAddress.toStdString(), m_clientID);
    m_conopts = new mqtt::connect_options();
    m_conopts->set_user_name(m_username.toStdString());
    m_conopts->set_password(m_password.toStdString());
    m_conopts->set_keep_alive_interval(45);
    //willmsg = new mqtt::message("muonpi/data/", "Last will and testament.", 1, true);
    //will = new mqtt::will_options(*willmsg);
    //conopts->set_will(*will)
    mqttConnect();
}

void MqttHandler::mqttConnect(){
    try {
        m_mqttClient->connect(*m_conopts)->wait();
        m_data_topic = new mqtt::topic(*m_mqttClient, "muonpi/data/"+m_username.toStdString()+"/"+m_stationID.toStdString(),m_qos);
        m_log_topic = new mqtt::topic(*m_mqttClient, "muonpi/log/"+m_username.toStdString()+"/"+m_stationID.toStdString(),m_qos);
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
        bool ok=pubtok->wait_for(m_timeout);
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
                bool ok=conntok->wait_for(m_timeout);
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
