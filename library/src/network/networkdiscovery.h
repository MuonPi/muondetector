#ifndef NETWORKDISCOVERY_H
#define NETWORKDISCOVERY_H

#include <cstdint>

class NetworkDiscovery {
public:
    enum class DeviceType {
        GUI,
        DAEMON
    };

    explicit NetworkDiscovery(DeviceType f_device_type, std::uint16_t f_port);
    ~NetworkDiscovery();

    void searchDevices();
    void readPendingDatagrams();
    // void foundDevices(const QList<QPair<quint16, QHostAddress>>& devices);

private:
    DeviceType m_device_type;
    std::uint16_t m_port;
    // QVector<QHostAddress> m_broadcast_address;
    // QVector<QHostAddress> m_own_ipv4;
    // QUdpSocket* socket;
    // QList<QPair<quint16, QHostAddress>> discovered_devices {};
};

#endif // NETWORKDISCOVERY_H
