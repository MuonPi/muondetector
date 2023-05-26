#include "networkdiscovery.h"
#include <QDataStream>
#include <QNetworkDatagram>
#include <QNetworkInterface>
#include <memory>

NetworkDiscovery::NetworkDiscovery(DeviceType f_device_type, quint16 f_port, QObject* parent)
    : QObject { parent }
    , m_device_type { f_device_type }
    , m_port { f_port }
    , socket { new QUdpSocket(this) }
{
    const QHostAddress& localhost = QHostAddress(QHostAddress::LocalHost);
    // qDebug() << QNetworkInterface::allAddresses();
    for (auto address : QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost && address != QHostAddress(QHostAddress::AnyIPv4)) {
            m_own_ipv4.push_back(address);
            auto temp = address.toString().split('.');
            QString broadcast_string {};
            for (auto it = temp.begin(); it != --temp.end(); it++) {
                broadcast_string += (*it + ".");
            }
            broadcast_string += "255";
            m_broadcast_address.push_back(QHostAddress { broadcast_string });
        }
    }

    connect(socket, &QUdpSocket::readyRead, this, &NetworkDiscovery::readPendingDatagrams);
    socket->bind(QHostAddress(QHostAddress::AnyIPv4), m_port, QUdpSocket::ShareAddress); // ShareAddress is important so both daemon and gui can be on same device
    // qDebug() << "broadcast address: " << m_broadcast_address.toString();
    // qDebug() << "listening on port: " << m_port;
}

void NetworkDiscovery::searchDevices()
{
    discovered_devices.clear();
    QByteArray data;
    auto dStream = std::make_unique<QDataStream>(&data, QIODevice::ReadWrite);
    (*dStream) << static_cast<quint16>(0x2a);
    (*dStream) << static_cast<quint16>(m_device_type);

    if (socket != nullptr) {
        // auto datagram = QNetworkDatagram{data,m_broadcast_address, m_port};
        // datagram.setHopLimit(255); // probably overkill
        qDebug() << "NetworkDiscovery is an experimental feature and may or may not work!";
        for (auto address : m_broadcast_address) {
            qDebug() << "NetworkDiscovery: sending " << data << " on address " << QHostAddress(address.toIPv4Address());
            socket->writeDatagram(data, address, m_port);
        }
    }
}

void NetworkDiscovery::readPendingDatagrams()
{
    while (socket->hasPendingDatagrams()) {
        auto datagram = socket->receiveDatagram();
        auto data = datagram.data();
        if (datagram.isNull() || !datagram.isValid() || data.size() < 4) {
            datagram.clear();
            return;
        }
        auto sender_address = QHostAddress(datagram.senderAddress().toIPv4Address());
        QDataStream inStream { &data, QIODevice::ReadOnly };
        quint16 the_answer_to_everything;
        quint16 device_type;
        inStream >> the_answer_to_everything;
        if (the_answer_to_everything != 0x2a) {
            datagram.clear();
            return;
        }
        inStream >> device_type;
        qDebug() << "found device: " << sender_address << " type: " << device_type;
        if (device_type == static_cast<quint16>(m_device_type)) {
            bool skip = false;
            for (auto address : m_own_ipv4) {
                if (address == sender_address) {
                    skip = true;
                }
            }
            if (skip) {
                continue; // do not answer or discover self
            }
        }
        discovered_devices.append(QPair<quint16, QHostAddress> { static_cast<quint16>(device_type), sender_address });

        if (static_cast<DeviceType>(device_type) == DeviceType::GUI) {
            data = QByteArray();
            QDataStream outStream { &data, QIODevice::ReadWrite };
            outStream << static_cast<quint16>(0x2a) << static_cast<quint16>(m_device_type);
            socket->writeDatagram(data, sender_address, m_port);
        }
    }
    emit foundDevices(discovered_devices);
}
