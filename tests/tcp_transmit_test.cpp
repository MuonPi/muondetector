#include "network/tcpserver.h"
#include "sinks/tcp_sink.h"
#include "sources/tcp_source.h"
#include "core/event_bus.h"
#include "core/thread_pool.h"
#include "data/ad1115_event.h"
#include "data/tcp_packet_event.h"
#include "ad1115.capnp.h"
#include "tcpmessage_keys.h"

#include <boost/asio.hpp>
#include <capnp/serialize.h>
#include <kj/array.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

using boost::asio::ip::tcp;

namespace {
// Wait until sink-side connection accounting reaches expected state.
bool waitForConnection(const TcpSink& sink, std::chrono::milliseconds timeout)
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (sink.connectionCount() > 0) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}
}

int main()
{
    // Build in-process server stack: acceptor + sink + source -> EventBus bridge.
    auto io = std::make_shared<boost::asio::io_context>();
    auto sink = std::make_shared<TcpSink>();
    TcpServer server(io, 0, sink);
    ThreadPool pool(2);
    EventBus bus(pool);
    TcpSource tcpSource(bus);

    std::promise<TcpPacketEvent> busPacketPromise;
    auto busPacketFuture = busPacketPromise.get_future();
    bus.subscribe<TcpPacketEvent>([&busPacketPromise](const TcpPacketEvent& event) {
        busPacketPromise.set_value(event);
    });
    server.addConnectionHandler([&tcpSource](const std::shared_ptr<TcpConnection>& conn) {
        tcpSource.registerConnection(conn);
    });

    std::thread ioThread([&io]() {
        io->run();
    });

    // Connect a client socket to the server under test.
    tcp::socket clientSocket(*io);
    boost::system::error_code ec;
    clientSocket.connect(tcp::endpoint(boost::asio::ip::address_v4::loopback(), server.port()), ec);
    if (ec) {
        std::cerr << "client connect failed: " << ec.message() << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    auto clientConn = std::make_shared<TcpConnection>(std::move(clientSocket));
    std::promise<TcpPacket> clientPacketPromise;
    auto clientPacketFuture = clientPacketPromise.get_future();
    clientConn->setPacketHandler([&clientPacketPromise](const TcpPacket& packet) {
        clientPacketPromise.set_value(packet);
    });
    clientConn->start();

    if (!waitForConnection(*sink, std::chrono::seconds(2))) {
        std::cerr << "timeout waiting for server-side connection registration\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    // Server -> client path: publish ADC event via sink and verify decoded payload.
    const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    Ads1115Event event{
        0x48,
        2,
        1234,
        1.2345f,
        static_cast<std::uint64_t>(now)
    };

    sink->handle(event);

    if (clientPacketFuture.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
        std::cerr << "timeout waiting for server->client packet\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    TcpPacket clientPacket = clientPacketFuture.get();
    if (clientPacket.key != static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_ADC_SAMPLE)) {
        std::cerr << "unexpected packet key from server: " << clientPacket.key << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    const std::size_t wordCount =
        (clientPacket.payload.size() + sizeof(capnp::word) - 1) / sizeof(capnp::word);
    auto alignedWords = kj::heapArray<capnp::word>(wordCount);
    std::memset(alignedWords.begin(), 0, wordCount * sizeof(capnp::word));
    std::memcpy(alignedWords.begin(), clientPacket.payload.data(), clientPacket.payload.size());

    capnp::FlatArrayMessageReader reader(alignedWords.asPtr());
    auto root = reader.getRoot<Ad1115Event>();

    if (root.getDeviceId() != event.deviceId ||
        root.getChannel() != event.channel ||
        root.getRawValue() != event.rawValue ||
        root.getTimestamp() != event.timestamp) {
        std::cerr << "decoded payload does not match source event\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    std::vector<std::uint8_t> pingPayload{ 'p', 'i', 'n', 'g' };
    // Client -> server path: packet must be forwarded into EventBus by TcpSource.
    clientConn->sendPacket(static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING), pingPayload);

    if (busPacketFuture.wait_for(std::chrono::seconds(2)) != std::future_status::ready) {
        std::cerr << "timeout waiting for client->server packet via EventBus\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    TcpPacketEvent busEvent = busPacketFuture.get();
    if (busEvent.packet.key != static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING)) {
        std::cerr << "unexpected packet key from client: " << busEvent.packet.key << "\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    if (busEvent.packet.payload != pingPayload) {
        std::cerr << "client->server payload mismatch\n";
        io->stop();
        ioThread.join();
        return 1;
    }

    io->stop();
    ioThread.join();
    return 0;
}
