#include "networkdiscovery.h"
#include <QNetworkInterface>
#include <QNetworkDatagram>
#include <QDataStream>

NetworkDiscovery::NetworkDiscovery(DeviceType f_device_type, quint16 f_bind_port, quint16 f_search_port, QObject *parent)
    : QObject{parent}
    , m_device_type{f_device_type}
    , m_bind_port{f_bind_port}
    , m_search_port{f_search_port}
    , socket{new QUdpSocket(this)}
{
//    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (auto address = QNetworkInterface::allAddresses().begin(); address != QNetworkInterface::allAddresses().end(); address++ ) {
//         if (address.protocol() == QAbstractsocket::IPv4Protocol && address != localhost && address != QHostAddress::AnyIPv4)
        if (address->isBroadcast())
        {
            m_broadcast_address = *address; // Must be tested if the correct address is found
        }
    }

    connect(socket, &QUdpSocket::readyRead, this, &NetworkDiscovery::readPendingDatagrams);
    socket->bind(QHostAddress::Any, m_bind_port);
}

void NetworkDiscovery::searchDevices()
{
    discovered_devices.clear();
    QByteArray data;
    auto dStream = QDataStream(&data, QIODevice::ReadWrite);
    dStream << static_cast<quint16>(m_device_type); // << socket->localAddress(); // not needed because already clear from the QNetworkDatagram

    if (socket != nullptr){
//        socket->writeDatagram(QNetworkDatagram{data,m_broadcast_address, m_port}); // this is supposedly not safe
        socket->writeDatagram(data, m_broadcast_address, m_search_port); // supposedly also not working... Must be tested
    }
}

void NetworkDiscovery::readPendingDatagrams()
{
    while (socket->hasPendingDatagrams()) {
            auto datagram = socket->receiveDatagram();
            auto data = datagram.data();
            QDataStream inStream{&data, QIODevice::ReadOnly};
            quint16 device_type;
            inStream >> device_type;
            discovered_devices.append(QPair{static_cast<quint16>(device_type), datagram.senderAddress()});
            emit foundDevices(discovered_devices);
            if (static_cast<DeviceType>(device_type) == DeviceType::GUI){
                data = QByteArray();
                QDataStream outStream{&data, QIODevice::ReadWrite};
                outStream << static_cast<quint16>(m_device_type);
                socket->writeDatagram(datagram.makeReply(data)); // supposedly not working
            }
        }
}
