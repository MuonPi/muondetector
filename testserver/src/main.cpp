#include "capnp/capnp_codec.h"
#include "core/component.h"
#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "core/thread_pool.h"
#include "data/events/tcp_packet_event.h"
#include "data/events/version_event.h"
#include "network/tcpconnection.h"
#include "network/tcpserver.h"
#include "sinks/tcp_sink.h"
#include "sources/tcp_source.h"

#include <boost/asio.hpp>
#include <capnp/serialize.h>
#include <iomanip>
#include <kj/array.h>
#include <memory>
#include <sstream>
#include <unordered_map>

import muondetector.tcpmessage_keys;

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

// void decode(EventBus& bus, const TcpPacketEvent& event) {
//     switch (static_cast<TCP_MSG_KEY>(event.packet.key)) {
//         case TCP_MSG_KEY::MSG_GPIO_EVENT:
//             bus.publish(CapnpCodec<GpioEvent>::decode(event.packet.payload));
//             break;
//         default:
//             print(event);
//             break;
//     }
// }

int main() {
    // Build in-process server stack: acceptor + sink + source -> EventBus bridge.
    auto io = std::make_shared<boost::asio::io_context>();
    auto sink = std::make_shared<TcpSink>();
    ThreadPool pool(2);
    EventBus bus(pool);

    bus.subscribe<TcpPacketEvent>(std::bind(&print, std::placeholders::_1));

    std::thread ioThread([&io]() { io->run(); });
    auto guard = boost::asio::make_work_guard(*io);

    auto tcp_sink = std::make_shared<TcpSink>();
    auto tcp_source = std::make_shared<TcpSource>(OtherComponent::TCP_SOURCE_0, bus);

    // --- tcp_server ---
    // When server accepts a new TCP connection, call this handler.
    TcpServer server(io, 51508, tcp_sink, &bus);
    server.addConnectionHandler([tcp_source](const std::shared_ptr<TcpConnection>& connection) {
        if (tcp_source != nullptr) {
            tcp_source->registerConnection(connection);
        } else {
            logError("Nullpointer in creating connection handler for tcpsource. Make sure "
                     "components are initialized "
                     "beforehand.");
        }
    });

    auto tcp_sink_weak = std::weak_ptr<TcpSink>(tcp_sink);
    bus.subscribe<VersionEvent>([tcp_sink_weak](const auto& event) {
        if (auto ptr = tcp_sink_weak.lock()) {
            ptr->handle(event);
        } else {
            std::cerr << "could not lock tcp_sink_weak" << std::endl;
        }
    });

    boost::asio::steady_timer timer(*io);
    timer.expires_after(std::chrono::seconds(1));
    timer.async_wait([&bus](boost::system::error_code) {
        std::cout << "send event";
        const MuonPi::Version::Version version{
            .major = 0, .minor = 0, .patch = 0, .additional = "test"};
        bus.publish(VersionEvent{.hw_ver = version, .sw_ver = version});
    });

    std::mutex mx;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lock(mx);
    cv.wait(lock);
    io->stop();
    ioThread.join();
    return EXIT_SUCCESS;
}
