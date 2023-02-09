#ifndef NETWORKDISCOVERY_H
#define NETWORKDISCOVERY_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>

class NetworkDiscovery : public QObject
{
    Q_OBJECT
public:
    enum class DeviceType {
        GUI,
        DAEMON
    };

    explicit NetworkDiscovery(DeviceType f_device_type, quint16 f_bind_port, quint16 f_search_port, QObject *parent = nullptr);


public slots:
    void searchDevices();
    void readPendingDatagrams();
signals:
    void foundDevices(const QList<QPair<quint16, QHostAddress>> &devices);
private:
    DeviceType m_device_type;
    quint16 m_bind_port;
    quint16 m_search_port;
    QHostAddress m_broadcast_address;
    QUdpSocket *socket;
    QList<QPair<quint16, QHostAddress>> discovered_devices{};
};

#endif // NETWORKDISCOVERY_H
