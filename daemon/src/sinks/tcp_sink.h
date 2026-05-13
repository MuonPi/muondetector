#ifndef TCP_SINK_H
#define TCP_SINK_H

#include "capnp/capnp_codec.h"
#include "core/logging/logger.h"
#include "data/events/ads1115_event.h"
#include "sink.h"
#include "tcpconnection.h"

#include <mutex>
#include <vector>

class TcpSink : public Sink {
  public:
    template <typename T>
    void handle(const T& msg);
    void addConnection(std::shared_ptr<TcpConnection> conn);
    void removeConnection(const std::shared_ptr<TcpConnection>& conn);
    void pruneDisconnected();
    auto connectionCount() const -> std::size_t;

  private:
    std::vector<std::shared_ptr<TcpConnection>> connections_;
    mutable std::mutex mutex_;
};

template <typename T>
void TcpSink::handle(const T& event) {
    std::vector<std::shared_ptr<TcpConnection>> conns;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conns = connections_; // copy for safe iteration
    }

    std::vector<std::uint8_t> packet = CapnpCodec<T>::encode(event);
    std::uint16_t key = CapnpCodec<T>::messageKey();

    logWarn("Send event: " + std::to_string(key) + " to " + std::to_string(conns.size()) +
            " connection(s)");

    for (std::size_t i = 0; i < conns.size(); ++i) {
        if (!conns[i])
            continue;

        if (i + 1 == conns.size())
            conns[i]->sendPacket(key, std::move(packet));
        else
            conns[i]->sendPacket(key, std::vector<uint8_t>(packet));
    }
}

#endif // TCP_SINK_H
