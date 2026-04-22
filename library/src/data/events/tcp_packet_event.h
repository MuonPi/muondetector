#ifndef TCP_PACKET_EVENT_H
#define TCP_PACKET_EVENT_H

#include "tcpconnection.h"

#include <memory>

struct TcpPacketEvent {
    std::shared_ptr<TcpConnection> connection;
    TcpPacket packet;
};

#endif // TCP_PACKET_EVENT_H
