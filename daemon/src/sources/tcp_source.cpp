#include "sources/tcp_source.h"
#include "data/tcp_packet_event.h"
#include "tcpconnection.h"

TcpSource::TcpSource(EventBus& bus)
    : bus_(bus)
{
}

void TcpSource::registerConnection(const std::shared_ptr<TcpConnection>& connection)
{
    if (!connection) {
        return;
    }

    auto weakConn = std::weak_ptr<TcpConnection>(connection);
    connection->setPacketHandler([this, weakConn](const TcpPacket& packet) {
        auto conn = weakConn.lock();
        if (!conn) {
            return;
        }
        bus_.publish(TcpPacketEvent{ conn, packet });
    });
}

void TcpSource::update()
{
    // network source is event-driven by socket callbacks
}
