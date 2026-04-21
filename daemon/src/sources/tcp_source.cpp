#include "sources/source.h"
#include "sources/tcp_source.h"
#include "data/events/tcp_packet_event.h"
#include "tcpconnection.h"

TcpSource::TcpSource(ComponentId id, EventBus& bus)
    : Source::Source(id), bus_(bus)
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
    if (!std::holds_alternative<OtherComponent>(id())) {
        throw std::logic_error("NonDeviceSource constructed with device ID");
    }
}
