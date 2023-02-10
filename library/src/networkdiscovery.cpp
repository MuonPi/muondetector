#include "networkdiscovery.h"
#include <QNetworkInterface>
#include <QNetworkDatagram>
#include <QDataStream>
#include <memory>

NetworkDiscovery::NetworkDiscovery(DeviceType f_device_type, quint16 f_port, QObject *parent)
    : QObject{parent}, m_device_type{f_device_type}, m_port{f_port}, socket{new QUdpSocket(this)}
{
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    qDebug() << QNetworkInterface::allAddresses();
    for (auto address : QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost && address != QHostAddress::AnyIPv4)
        {
            m_own_ipv4 = address;
            auto temp = address.toString().split('.');
            QString broadcast_string{};
            for (auto it = temp.begin(); it != --temp.end(); it++){
                broadcast_string += (*it + ".");
            }
            broadcast_string += "255";
            m_broadcast_address = QHostAddress{broadcast_string};
        }
    }

    connect(socket, &QUdpSocket::readyRead, this, &NetworkDiscovery::readPendingDatagrams);
    socket->bind(QHostAddress::Any, m_port, QUdpSocket::ShareAddress); // ShareAddress is important so both daemon and gui can be on same device
    // qDebug() << "broadcast address: " << m_broadcast_address.toString();
    // qDebug() << "listening on port: " << m_port;
}

void NetworkDiscovery::searchDevices()
{
    discovered_devices.clear();
    QByteArray data;
    auto dStream = std::make_unique<QDataStream>(&data, QIODevice::ReadWrite);
    (*dStream) << static_cast<quint16>(m_device_type);

    if (socket != nullptr)
    {
        // qDebug() << "sending " << data;
        // auto datagram = QNetworkDatagram{data,m_broadcast_address, m_port};
        // datagram.setHopLimit(255); // probably overkill
        socket->writeDatagram(data, m_broadcast_address, m_port);
    }
}

void NetworkDiscovery::readPendingDatagrams()
{
    while (socket->hasPendingDatagrams())
    {
        auto datagram = socket->receiveDatagram();
        auto sender_address = QHostAddress(datagram.senderAddress().toIPv4Address());
        auto data = datagram.data();
        QDataStream inStream{&data, QIODevice::ReadOnly};
        quint16 device_type;
        inStream >> device_type;
        if ( sender_address == m_own_ipv4 && device_type == static_cast<quint16>(m_device_type)){
            continue; // do not answer or discover self
        }
        discovered_devices.append(QPair<quint16, QHostAddress>{static_cast<quint16>(device_type), sender_address});
        emit foundDevices(discovered_devices);

        if (static_cast<DeviceType>(device_type) == DeviceType::GUI)
        {
            data = QByteArray();
            QDataStream outStream{&data, QIODevice::ReadWrite};
            outStream << static_cast<quint16>(m_device_type);
            socket->writeDatagram(data, sender_address, m_port);
        }
    }
}
