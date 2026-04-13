#include "network/tcpserver.h"
#include "sinks/tcp_sink.h"
#include "sources/tcp_source.h"
#include "core/event_bus.h"
#include "core/thread_pool.h"
#include "data/tcp_packet_event.h"
#include "tcpconnection.h"
#include "tcpmessage_keys.h"

#include <boost/asio.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <future>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

namespace {
// Big-endian helpers for handcrafted malformed frame.
auto be16(std::uint16_t v) -> std::array<std::uint8_t, 2>
{
    return {
        static_cast<std::uint8_t>((v >> 8) & 0xFF),
        static_cast<std::uint8_t>(v & 0xFF)
    };
}

auto be32(std::uint32_t v) -> std::array<std::uint8_t, 4>
{
    return {
        static_cast<std::uint8_t>((v >> 24) & 0xFF),
        static_cast<std::uint8_t>((v >> 16) & 0xFF),
        static_cast<std::uint8_t>((v >> 8) & 0xFF),
        static_cast<std::uint8_t>(v & 0xFF)
    };
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
    // Wire server with TcpSource so inbound packets are published into EventBus.
    auto io = std::make_shared<boost::asio::io_context>();
    auto sink = std::make_shared<TcpSink>();
    TcpServer server(io, 0, sink);
    ThreadPool pool(2);
    EventBus bus(pool);
    TcpSource tcpSource(NonDeviceSource::TCP_SOURCE_0, bus);

    server.addConnectionHandler([&tcpSource](const std::shared_ptr<TcpConnection>& conn) {
        tcpSource.registerConnection(conn);
    });

    std::mutex eventsMutex;
    std::vector<std::string> payloads;
    std::promise<void> gotTwoMessages;
    auto gotTwoMessagesFuture = gotTwoMessages.get_future();
    std::atomic<bool> promiseDone{false};

    bus.subscribe<TcpPacketEvent>(
        [&](const TcpPacketEvent& e) {
            if (e.packet.key != static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING)) {
                return;
            }

            std::string value(e.packet.payload.begin(), e.packet.payload.end());
            std::lock_guard<std::mutex> lock(eventsMutex);
            payloads.push_back(value);

            if (payloads.size() >= 2 && !promiseDone.exchange(true)) {
                gotTwoMessages.set_value();
            }
        });

    std::thread ioThread([&io]() {
        io->run();
    });

    // Connect healthy client #1 and malformed client #2.
    boost::system::error_code ec;

    tcp::socket client1Socket(*io);
    client1Socket.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), server.port()), ec);
    if (ec) {
        std::cerr << "client1 connect failed: " << ec.message() << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    auto client1 = std::make_shared<TcpConnection>(std::move(client1Socket));
    client1->start();

    tcp::socket client2(*io);
    client2.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), server.port()), ec);
    if (ec) {
        std::cerr << "client2 connect failed: " << ec.message() << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    if (!waitForConnectionCount(*sink, 2, std::chrono::seconds(2))) {
        std::cerr << "expected two active connections\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    client1->sendPacket(static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING),
                        std::vector<std::uint8_t>{'o', 'k', '1'});

    // Send malformed frame from client #2 to trigger isolation cleanup.
    constexpr std::uint32_t magic = 0x4D554F4Eu;
    constexpr std::uint16_t version = 1;
    constexpr std::uint16_t key = 2;
    const std::vector<std::uint8_t> badPayload{'b', 'a', 'd'};
    constexpr std::uint32_t wrongChecksum = 0xABCDEF01u;
    std::vector<std::uint8_t> badFrame;
    const auto m = be32(magic);
    const auto v = be16(version);
    const auto k = be16(key);
    const auto s = be32(static_cast<std::uint32_t>(badPayload.size()));
    const auto c = be32(wrongChecksum);
    badFrame.insert(badFrame.end(), m.begin(), m.end());
    badFrame.insert(badFrame.end(), v.begin(), v.end());
    badFrame.insert(badFrame.end(), k.begin(), k.end());
    badFrame.insert(badFrame.end(), s.begin(), s.end());
    badFrame.insert(badFrame.end(), badPayload.begin(), badPayload.end());
    badFrame.insert(badFrame.end(), c.begin(), c.end());
    boost::asio::write(client2, boost::asio::buffer(badFrame), ec);
    if (ec) {
        std::cerr << "client2 malformed write failed: " << ec.message() << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    // One client should remain (healthy client #1).
    if (!waitForConnectionCount(*sink, 1, std::chrono::seconds(2))) {
        std::cerr << "malformed client did not get isolated/cleaned\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    // Healthy client must still work after other client is dropped.
    client1->sendPacket(static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING),
                        std::vector<std::uint8_t>{'o', 'k', '2'});

    if (gotTwoMessagesFuture.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
        std::cerr << "did not receive expected messages from healthy client\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    {
        std::lock_guard<std::mutex> lock(eventsMutex);
        if (payloads.size() < 2 || payloads[0] != "ok1" || payloads[1] != "ok2") {
            std::cerr << "message sequence mismatch for healthy client\n";
            io->stop();
            ioThread.join();
            return 1;
        }
    }

    io->stop();
    ioThread.join();
    return 0;
}
