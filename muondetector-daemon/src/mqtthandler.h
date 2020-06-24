#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include <QObject>
#include <QTimer>
#include <QPointer>
#include <async_client.h>
#include <connect_options.h>
#include <string>
#include <config.h>

class callback : public virtual mqtt::callback{
public:
    void connection_lost(const std::string& cause) override;
    void delivery_complete(mqtt::delivery_token_ptr tok) override;
};

class action_listener : public virtual mqtt::iaction_listener{
protected:
    void on_failure(const mqtt::token& tok) override;
    void on_success(const mqtt::token& tok) override;
};

class delivery_action_listener : public action_listener{
public:
    delivery_action_listener() : done_(false) {}
    void on_failure(const mqtt::token& tok) override;
    void on_success(const mqtt::token& tok) override;
    bool is_done() const { return done_; }
private:
    std::atomic<bool> done_;
};

class MqttHandler : public QObject
{
    Q_OBJECT

    signals:
        void mqttConnectionStatus(bool connected);

    public slots:
        void start(QString username, QString password);
        void sendData(const QString &message);
        void sendLog(const QString &message);
        void onRequestConnectionStatus();

    public:
        MqttHandler(QString station_ID, int verbosity=0);
        //using QObject::QObject;
        void mqttStartConnection();
        void mqttDisconnect();

    private:
        void mqttConnect();
        const int qos = MUONPI_MQTT_QOS;
        int timeout = MUONPI_MQTT_TIMEOUT_MS;
        QPointer<QTimer> reconnectTimer;
        mqtt::async_client *mqttClient = nullptr;
        mqtt::topic *data_topic = nullptr;
        mqtt::topic *log_topic = nullptr;
        mqtt::connect_options *conopts = nullptr;
        mqtt::message *willmsg = nullptr;
        mqtt::will_options *will = nullptr;
        bool _mqttConnectionStatus = false;
        QString mqttAddress = MUONPI_MQTT_SERVER;
        QString stationID="0";
        QString username;
        QString password;
        std::string clientID;
		int verbose=0;
};

#endif // MQTTHANDLER_H
