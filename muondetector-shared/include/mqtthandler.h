#ifndef MQTTHANDLER_H
#define MQTTHANDLER_H

#include "muondetector_shared_global.h"
#include "config.h"

#include <QObject>
#include <QTimer>
#include <QPointer>
#include <async_client.h>
#include <connect_options.h>
#include <string>

namespace MuonPi {

class MUONDETECTORSHARED callback : public virtual mqtt::callback{
public:
    void connection_lost(const std::string& cause) override;
    void delivery_complete(mqtt::delivery_token_ptr tok) override;
};

class MUONDETECTORSHARED action_listener : public virtual mqtt::iaction_listener{
protected:
    void on_failure(const mqtt::token& tok) override;
    void on_success(const mqtt::token& tok) override;
};

class MUONDETECTORSHARED delivery_action_listener : public action_listener{
public:
    void on_failure(const mqtt::token& tok) override;
    void on_success(const mqtt::token& tok) override;
    bool is_done() const { return m_done; }

private:
    std::atomic<bool> m_done { false };
};

class MUONDETECTORSHARED MqttHandler : public QObject
{
    Q_OBJECT

    signals:
        void mqttConnectionStatus(bool connected);

    public slots:
        void start(const QString& username, const QString& password);
        void sendData(const QString &message);
        void sendLog(const QString &message);
        void onRequestConnectionStatus();

    public:
        MqttHandler(const QString& station_ID, const int verbosity=0);
        //using QObject::QObject;
        void mqttStartConnection();
        void mqttDisconnect();

    private:
        void mqttConnect();
        QPointer<QTimer>  m_reconnectTimer {};
        mqtt::async_client *m_mqttClient { nullptr };
        mqtt::topic *m_data_topic { nullptr };
        mqtt::topic *m_log_topic { nullptr };
        mqtt::connect_options *m_conopts { nullptr };
        mqtt::message *m_willmsg { nullptr };
        mqtt::will_options *m_will { nullptr };
        bool m_mqttConnectionStatus { false };
        std::string m_stationID { "0" };
        std::string m_username {};
        std::string m_password {};
        std::string m_clientID {};
        int m_verbose { 0 };
};
} // namespace MuonPi

#endif // MQTTHANDLER_H
