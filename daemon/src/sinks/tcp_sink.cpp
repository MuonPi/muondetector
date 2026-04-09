#include "tcp_sink.h"
#include "sink.h"
#include "core/event_bus.h"
#include "tcpconnection.h"
#include "tcpmessage_keys.h"
#include "ad1115.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

#include <vector>
#include <mutex>
#include <cstring>
#include <algorithm>


void TcpSink::addConnection(std::shared_ptr<TcpConnection> conn)
{
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.push_back(conn);
}

void TcpSink::removeConnection(const std::shared_ptr<TcpConnection>& conn)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove(connections_.begin(), connections_.end(), conn);
    connections_.erase(it, connections_.end());
}

void TcpSink::pruneDisconnected()
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = std::remove_if(connections_.begin(), connections_.end(),
                             [](const std::shared_ptr<TcpConnection>& conn) {
                                 return !conn || !conn->isOpen();
                             });
    connections_.erase(it, connections_.end());
}

auto TcpSink::connectionCount() const -> std::size_t
{
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.size();
}

void TcpSink::handle(const Ad1115SampleEvent& event)
{
    std::vector<std::shared_ptr<TcpConnection>> conns;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        conns = connections_; // copy for safe iteration
    }

    auto packet = serialize(event);

    for (auto& conn : conns)
    {
        if (conn)
            conn->sendPacket(static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_ADC_SAMPLE), packet);
    }
}

std::vector<uint8_t> TcpSink::serialize(const Ad1115SampleEvent& event)
{
    capnp::MallocMessageBuilder msg;
    auto root = msg.initRoot<Ad1115Event>();

    root.setDeviceId(event.deviceId);
    root.setChannel(event.channel);
    root.setRawValue(event.rawValue);
    root.setVoltage(event.voltage);
    root.setTimestamp(event.timestamp);

    auto flat = capnp::messageToFlatArray(msg);
    auto bytes = flat.asBytes();

    return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
}
