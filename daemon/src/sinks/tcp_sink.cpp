#include "tcp_sink.h"
#include "sink.h"
#include "core/event_bus.h"
#include "tcpconnection.h"
#include "ad1115.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

#include <vector>
#include <mutex>
#include <cstring>


void TcpSink::addConnection(std::shared_ptr<TcpConnection> conn)
{
    std::lock_guard<std::mutex> lock(mutex_);
    connections_.push_back(conn);
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
            conn->send(packet);
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

    uint32_t size = bytes.size();

    std::vector<uint8_t> buffer(sizeof(size) + size);
    std::memcpy(buffer.data(), &size, sizeof(size));
    std::memcpy(buffer.data() + sizeof(size), bytes.begin(), size);

    return buffer;
}
