#include "tcp_sink.h"

#include "core/event_bus.h"
#include "data/events/ads1115_event.h"
#include "sink.h"
#include "tcpconnection.h"
#include "tcpmessage_keys.h"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>

void TcpSink::addConnection(std::shared_ptr<TcpConnection> conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.push_back(conn);
}

void TcpSink::removeConnection(const std::shared_ptr<TcpConnection>& conn) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove(connections_.begin(), connections_.end(), conn);
    connections_.erase(it, connections_.end());
}

void TcpSink::pruneDisconnected() {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(
        connections_.begin(), connections_.end(),
        [](const std::shared_ptr<TcpConnection>& conn) { return !conn || !conn->isOpen(); });
    connections_.erase(it, connections_.end());
}

auto TcpSink::connectionCount() const -> std::size_t {
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.size();
}
