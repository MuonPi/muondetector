#include "network/tcpserver.h"
#include "sinks/tcp_sink.h"
#include "tcpconnection.h"
#include "tcpmessage_keys.h"

#include <boost/asio.hpp>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

using boost::asio::ip::tcp;

namespace {
// Wait until sink-side connection count reaches expected value.
bool waitForConnectionCount(const TcpSink& sink, std::size_t expected,
                            std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (sink.connectionCount() == expected) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}
} // namespace

int main() {
    // Start server and attach a client-side TcpConnection packet handler.
    auto io = std::make_shared<boost::asio::io_context>();
    auto sink = std::make_shared<TcpSink>();
    TcpServer server(io, 0, sink);

    std::promise<TcpPacket> packetPromise;
    auto packetFuture = packetPromise.get_future();

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
    conn->setPacketHandler([&](const TcpPacket& p) { packetPromise.set_value(p); });
    conn->start();

    if (!waitForConnectionCount(*sink, 1, std::chrono::seconds(2))) {
        std::cerr << "connection not registered\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    // Trigger one heartbeat round and verify MSG_PING arrives.
    server.heartbeatAndCleanup(std::chrono::seconds(10));

    if (packetFuture.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
        std::cerr << "no heartbeat ping received\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    const auto packet = packetFuture.get();
    if (packet.key != static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING)) {
        std::cerr << "unexpected heartbeat key: " << packet.key << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    io->stop();
    ioThread.join();
    return 0;
}
