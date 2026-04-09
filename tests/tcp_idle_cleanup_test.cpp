#include "network/tcpserver.h"
#include "sinks/tcp_sink.h"

#include <boost/asio.hpp>

#include <chrono>
#include <iostream>
#include <thread>

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
    // Start server and create one idle client connection.
    auto io = std::make_shared<boost::asio::io_context>();
    auto sink = std::make_shared<TcpSink>();
    TcpServer server(io, 0, sink);

    std::thread ioThread([&io]() {
        io->run();
    });

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
        std::cerr << "server did not register connection\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    // Let connection become stale and trigger server cleanup logic.
    std::this_thread::sleep_for(std::chrono::milliseconds(25));
    server.heartbeatAndCleanup(std::chrono::milliseconds(5));

    if (!waitForConnectionCount(*sink, 0, std::chrono::seconds(2))) {
        std::cerr << "idle connection was not cleaned up\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    io->stop();
    ioThread.join();
    return 0;
}
