#ifndef NETWORKDISCOVERY_H
#define NETWORKDISCOVERY_H

#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QVector>

class NetworkDiscovery : public QObject {
    Q_OBJECT
public:
    enum class DeviceType {
        GUI,
        DAEMON
    };

    explicit NetworkDiscovery(DeviceType f_device_type, quint16 f_port, QObject* parent = nullptr);

public slots:
    void searchDevices();
    void readPendingDatagrams();
signals:
    void foundDevices(const QList<QPair<quint16, QHostAddress>>& devices);

private:
    DeviceType m_device_type;
    quint16 m_port;
    QVector<QHostAddress> m_broadcast_address;
    QVector<QHostAddress> m_own_ipv4;
    QUdpSocket* socket;
    QList<QPair<quint16, QHostAddress>> discovered_devices {};
};

#endif // NETWORKDISCOVERY_H
