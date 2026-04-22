#include "network/tcpserver.h"
#include "sinks/tcp_sink.h"

#include <array>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

namespace {
// Framing helpers for crafting protocol-invalid packets.
auto be16(std::uint16_t v) -> std::array<std::uint8_t, 2> {
    return {static_cast<std::uint8_t>((v >> 8) & 0xFF), static_cast<std::uint8_t>(v & 0xFF)};
}

auto be32(std::uint32_t v) -> std::array<std::uint8_t, 4> {
    return {static_cast<std::uint8_t>((v >> 24) & 0xFF),
            static_cast<std::uint8_t>((v >> 16) & 0xFF), static_cast<std::uint8_t>((v >> 8) & 0xFF),
            static_cast<std::uint8_t>(v & 0xFF)};
}

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
    // Bring up server and connect one client.
    auto io = std::make_shared<boost::asio::io_context>();
    auto sink = std::make_shared<TcpSink>();
    TcpServer server(io, 0, sink);
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

    std::vector<std::uint8_t> frame;
    const auto m = be32(0x4D554F4Eu);
    // Unsupported protocol version must force disconnect.
    const auto v = be16(2); // bad version
    const auto k = be16(2);
    const auto s = be32(0);
    const auto c = be32(2166136261u);
    frame.insert(frame.end(), m.begin(), m.end());
    frame.insert(frame.end(), v.begin(), v.end());
    frame.insert(frame.end(), k.begin(), k.end());
    frame.insert(frame.end(), s.begin(), s.end());
    frame.insert(frame.end(), c.begin(), c.end());

    boost::asio::write(client, boost::asio::buffer(frame), ec);
    if (ec) {
        std::cerr << "write failed: " << ec.message() << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    if (!waitForConnectionCount(*sink, 0, std::chrono::seconds(2))) {
        std::cerr << "bad version did not trigger cleanup\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    io->stop();
    ioThread.join();
    return 0;
}
