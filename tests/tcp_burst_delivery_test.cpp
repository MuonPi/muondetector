#include "network/tcpserver.h"
#include "sinks/tcp_sink.h"
#include "sources/tcp_source.h"
#include "core/event_bus.h"
#include "core/thread_pool.h"
#include "data/tcp_packet_event.h"
#include "tcpconnection.h"
#include "tcpmessage_keys.h"

#include <boost/asio.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

namespace {
// Wait until sink-side connection count reaches expected value.
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
    constexpr int kBurstCount = 100;

    // Server + source + EventBus path under burst traffic.
    auto io = std::make_shared<boost::asio::io_context>();
    auto sink = std::make_shared<TcpSink>();
    TcpServer server(io, 0, sink);
    ThreadPool pool(4);
    EventBus bus(pool);
    TcpSource tcpSource(bus);
    server.addConnectionHandler([&tcpSource](const std::shared_ptr<TcpConnection>& conn) {
        tcpSource.registerConnection(conn);
    });

    // Count only our test marker packets.
    std::atomic<int> received{0};
    bus.subscribe<TcpPacketEvent>([&](const TcpPacketEvent& e) {
        if (e.packet.key == static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING) &&
            e.packet.payload.size() == 1 && e.packet.payload[0] == 0x2A) {
            received.fetch_add(1);
        }
    });

    std::thread ioThread([&io]() { io->run(); });

    tcp::socket socket(*io);
    boost::system::error_code ec;
    socket.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), server.port()), ec);
    if (ec) {
        std::cerr << "connect failed: " << ec.message() << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    auto conn = std::make_shared<TcpConnection>(std::move(socket));
    conn->start();

    if (!waitForConnectionCount(*sink, 1, std::chrono::seconds(2))) {
        std::cerr << "connection not registered\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    // Send a short burst from one client connection.
    const std::vector<std::uint8_t> payload{0x2A};
    for (int i = 0; i < kBurstCount; ++i) {
        conn->sendPacket(static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING), payload);
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (std::chrono::steady_clock::now() < deadline) {
        if (received.load() == kBurstCount) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (received.load() != kBurstCount) {
        std::cerr << "burst delivery mismatch: expected " << kBurstCount
                  << " got " << received.load() << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    io->stop();
    ioThread.join();
    return 0;
}
