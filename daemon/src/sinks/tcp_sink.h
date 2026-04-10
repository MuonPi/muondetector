#ifndef TCP_SINK_H
#define TCP_SINK_H

#include "sink.h"
#include "data/ad1115_event.h"
#include "tcpconnection.h"

#include <vector>
#include <mutex>


class TcpSink : public Sink
{
public:
    void handle(const Ads1115Event& event);
    void addConnection(std::shared_ptr<TcpConnection> conn);
    void removeConnection(const std::shared_ptr<TcpConnection>& conn);
    void pruneDisconnected();
    auto connectionCount() const -> std::size_t;

private:
    std::vector<uint8_t> serialize(const Ads1115Event& event);

private:
    std::vector<std::shared_ptr<TcpConnection>> connections_;
    mutable std::mutex mutex_;
};
#endif // TCP_SINK_H
