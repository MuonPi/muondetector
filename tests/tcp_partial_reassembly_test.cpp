#include "network/tcpserver.h"
#include "sinks/tcp_sink.h"
#include "sources/tcp_source.h"
#include "core/event_bus.h"
#include "core/thread_pool.h"
#include "data/events/tcp_packet_event.h"
#include "tcpmessage_keys.h"

#include <boost/asio.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

namespace {
// Big-endian framing helpers + checksum matching TcpConnection implementation.
auto be16(std::uint16_t v) -> std::array<std::uint8_t, 2>
{
    return {static_cast<std::uint8_t>((v >> 8) & 0xFF), static_cast<std::uint8_t>(v & 0xFF)};
}

auto be32(std::uint32_t v) -> std::array<std::uint8_t, 4>
{
    return {
        static_cast<std::uint8_t>((v >> 24) & 0xFF),
        static_cast<std::uint8_t>((v >> 16) & 0xFF),
        static_cast<std::uint8_t>((v >> 8) & 0xFF),
        static_cast<std::uint8_t>(v & 0xFF)};
}

auto checksum(const std::vector<std::uint8_t>& payload) -> std::uint32_t
{
    std::uint32_t hash = 2166136261u;
    for (auto b : payload) {
        hash ^= b;
        hash *= 16777619u;
    }
    return hash;
}

bool waitForConnectionCount(const TcpSink& sink, std::size_t expected, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (sink.connectionCount() == expected) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}
}

int main()
{
    // Server + source + EventBus path under test.
    auto io = std::make_shared<boost::asio::io_context>();
    auto sink = std::make_shared<TcpSink>();
    TcpServer server(io, 0, sink);
    ThreadPool pool(2);
    EventBus bus(pool);
    TcpSource tcpSource(NonDeviceComponent::TCP_SOURCE_0, bus);
    server.addConnectionHandler([&tcpSource](const std::shared_ptr<TcpConnection>& conn) {
        tcpSource.registerConnection(conn);
    });

    std::promise<TcpPacketEvent> packetPromise;
    auto packetFuture = packetPromise.get_future();
    std::atomic<bool> received{false};
    bus.subscribe<TcpPacketEvent>([&](const TcpPacketEvent& e) {
        if (!received.exchange(true)) {
            packetPromise.set_value(e);
        }
    });

    std::thread ioThread([&io]() { io->run(); });

    tcp::socket client(*io);
    boost::system::error_code ec;
    client.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), server.port()), ec);
    if (ec) {
        std::cerr << "connect failed: " << ec.message() << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }
    if (!waitForConnectionCount(*sink, 1, std::chrono::seconds(2))) {
        std::cerr << "connection not registered\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    // Build one valid frame and split it into two writes to force reassembly.
    const std::vector<std::uint8_t> payload{'f', 'r', 'a', 'g'};
    std::vector<std::uint8_t> frame;
    const auto m = be32(0x4D554F4Eu);
    const auto v = be16(1);
    const auto k = be16(static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING));
    const auto s = be32(static_cast<std::uint32_t>(payload.size()));
    const auto c = be32(checksum(payload));
    frame.insert(frame.end(), m.begin(), m.end());
    frame.insert(frame.end(), v.begin(), v.end());
    frame.insert(frame.end(), k.begin(), k.end());
    frame.insert(frame.end(), s.begin(), s.end());
    frame.insert(frame.end(), payload.begin(), payload.end());
    frame.insert(frame.end(), c.begin(), c.end());

    const std::size_t split = 5; // header fragment cut point
    boost::asio::write(client, boost::asio::buffer(frame.data(), split), ec);
    if (ec) {
        std::cerr << "write chunk1 failed: " << ec.message() << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    boost::asio::write(client, boost::asio::buffer(frame.data() + split, frame.size() - split), ec);
    if (ec) {
        std::cerr << "write chunk2 failed: " << ec.message() << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    // Packet should be emitted once full frame is received.
    if (packetFuture.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
        std::cerr << "did not receive reassembled packet\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    const auto event = packetFuture.get();
    if (event.packet.key != static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING) || event.packet.payload != payload) {
        std::cerr << "reassembled packet mismatch\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    io->stop();
    ioThread.join();
    return 0;
}
