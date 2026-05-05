#include "capnp/capnp_codec.h"
#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/thread_pool.h"
#include "data/events/ads1115_event.h"
#include "data/events/tcp_packet_event.h"
#include "data/gpio_pin_definitions.h"
#include "network/tcpconnection.h"
#include "network/tcpmessage_keys.h"
#include "sinks/tcp_sink.h"
#include "sources/tcp_source.h"

#include <boost/asio.hpp>
#include <capnp/serialize.h>
#include <iomanip>
#include <kj/array.h>
#include <sstream>
#include <unordered_map>

using boost::asio::ip::tcp;

auto print(const TcpPacketEvent& event) -> std::string {
    std::stringstream sstr{};
    // Information from socket
    const tcp::socket& socket = event.connection->socket();
    std::string remote_address = "unknown";
    if (socket.is_open()) {
        boost::system::error_code ec;
        remote_address = socket.remote_endpoint(ec).address().to_string();
    }
    auto local_address = socket.local_endpoint().address().to_string();
    auto is_open = socket.is_open();

    // Output
    sstr << "----------------------\n";
    sstr << "Received TCP Message:\n";
    sstr << "From: " << remote_address << "\n";
    sstr << "On: " << local_address << "\n";
    sstr << "Is Open: " << (is_open ? "true" : "false") << "\n";
    sstr << "Key: 0x" << std::setw(2) << std::setfill('0') << std::hex << event.packet.key
         << std::dec << "\n";
    sstr << "Payload:\n";
    for (auto byte : event.packet.payload) {
        sstr << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned>(byte);
    }
    sstr << std::dec << "\n----------------------\n";
    return sstr.str();
}

void decode(EventBus& bus, const TcpPacketEvent& event) {
    switch (static_cast<TCP_MSG_KEY>(event.packet.key)) {
        case TCP_MSG_KEY::MSG_GPIO_EVENT:
            bus.publish(CapnpCodec<GpioEvent>::decode(event.packet.payload));
            break;
        default:
            print(event);
            break;
    }
}

int main() {
    // Build in-process server stack: acceptor + sink + source -> EventBus bridge.
    auto io = std::make_shared<boost::asio::io_context>();
    auto sink = std::make_shared<TcpSink>();
    ThreadPool pool(2);
    EventBus bus(pool);
    TcpSource tcpSource(OtherComponent::TCP_SOURCE_0, bus);

    bus.subscribe<GpioEvent>([](const GpioEvent& event) {
        logInfo("GpioEvent: " + std::to_string(event.gpio_pin) +
                " edge: " + (event.edge == EventEdge::Rising ? "rising" : "falling"));
    });
    bus.subscribe<TcpPacketEvent>([&bus](const TcpPacketEvent& event) { decode(bus, event); });

    std::thread ioThread([&io]() { io->run(); });
    auto guard = boost::asio::make_work_guard(*io);

    tcp::socket clientSocket(*io);
    boost::system::error_code ec;
    auto server_ip = boost::asio::ip::make_address_v4("192.168.2.16", ec);
    if (ec) {
        logError("Invalid IP: " + ec.message());
    }
    tcp::endpoint endpoint(server_ip, 51508);
    clientSocket.connect(endpoint, ec);
    if (ec) {
        logError("client connect failed: " + ec.message());
        io->stop();
        ioThread.join();
        return EXIT_FAILURE;
    }

    auto clientConn = std::make_shared<TcpConnection>(std::move(clientSocket));
    auto weakConn = std::weak_ptr<TcpConnection>(clientConn);
    clientConn->setPacketHandler([&bus, weakConn](const TcpPacket& packet) {
        if (auto conn = weakConn.lock()) {
            bus.publish(TcpPacketEvent{.connection = conn, .packet = packet});
        }
    });
    clientConn->start();
    std::mutex mx;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lock(mx);
    cv.wait(lock);
    io->stop();
    ioThread.join();
    return EXIT_SUCCESS;
}