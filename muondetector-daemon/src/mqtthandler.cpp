#include <mqtthandler.h>
#include <QDebug>

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
    done_ = true;
}

void delivery_action_listener::on_success(const mqtt::token& tok) {
    action_listener::on_success(tok);
    done_ = true;
}

MqttHandler::MqttHandler(){
}

void MqttHandler::start(QString username, QString password, QString station_ID){
    this->username = username;
    this->password = password;
    this->stationID = station_ID;
    mqttConnect();
}

void MqttHandler::mqttConnect(){
    try {
        mqttClient = new mqtt::async_client(mqttAddress.toStdString(), username.toStdString());
        conopts = new mqtt::connect_options();
        conopts->set_user_name(username.toStdString());
        conopts->set_password(password.toStdString());
        conopts->set_keep_alive_interval(45);
        //willmsg = new mqtt::message("muonpi/data/", "Last will and testament.", 1, true);
        //will = new mqtt::will_options(*willmsg);
        //conopts->set_will(*will);
        mqttClient->connect(*conopts)->wait();
        data_topic = new mqtt::topic(*mqttClient, "muonpi/data/"+username.toStdString()+"/"+stationID.toStdString(),qos);
        log_topic = new mqtt::topic(*mqttClient, "muonpi/log/"+username.toStdString()+"/"+stationID.toStdString(),qos);
        emit mqttConnectionStatus(true);
        _mqttConnectionStatus = true;
        qDebug() << "MQTT connected";
    }
    catch (const mqtt::exception& exc) {
        std::cerr << exc.what() << std::endl;
    }
}

void MqttHandler::mqttDisconnect(){
    // Disconnect
    if (mqttClient == nullptr){
        return;
    }
    try {
        mqtt::token_ptr conntok = mqttClient->disconnect();
        conntok->wait();
        emit mqttConnectionStatus(false);
        _mqttConnectionStatus = false;
    }
    catch (const mqtt::exception& exc) {
        std::cerr << exc.what() << std::endl;
    }
}

void MqttHandler::sendData(const QString &message){
    if (data_topic == nullptr || log_topic == nullptr){
        return;
    }
    try {
        data_topic->publish(message.toStdString())->wait();
    }
    catch (const mqtt::exception& exc) {
        std::cerr << exc.what() << std::endl;
    }
}

void MqttHandler::sendLog(const QString &message){
    if (log_topic == nullptr){
        return;
    }
    try {
        log_topic->publish(message.toStdString());
    }
    catch (const mqtt::exception& exc) {
        std::cerr << exc.what() << std::endl;
    }
}

void MqttHandler::onRequestConnectionStatus(){
    emit mqttConnectionStatus(_mqttConnectionStatus);
}
