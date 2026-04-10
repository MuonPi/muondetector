#include "network/tcpconnection.h"
#include "sinks/tcp_sink.h"
#include "sources/tcp_source.h"
#include "core/event_bus.h"
#include "core/thread_pool.h"
#include "data/ad1115_event.h"
#include "data/tcp_packet_event.h"
#include "core/logging/logger.h"

#include <boost/asio.hpp>
#include <capnp/serialize.h>
#include <kj/array.h>
#include <sstream>
#include <iomanip>


using boost::asio::ip::tcp;

std::string build_message(const TcpPacketEvent& event)
{
    std::stringstream sstr{};
    // Information from socket
    const tcp::socket& socket = event.connection->socket();
    auto remote_address = socket.remote_endpoint().address().to_string();
    auto local_address = socket.local_endpoint().address().to_string();
    auto is_open = socket.is_open();
    std::size_t bytes_available = socket.available();

    // Output
    sstr << "----------------------\n";
    sstr << "Received TCP Message:\n";
    sstr << "From: " << remote_address << "\n";
    sstr << "On: " << local_address<< "\n";
    sstr << "Is Open: " << (is_open ? "true" : "false") << "\n";
    sstr << "Bytes available: " << bytes_available<< "\n";
    sstr << "Key: 0x" << std::setw(2) << std::setfill('0') << std::hex << event.packet.key << std::dec << "\n";
    sstr << "Payload:\n";
    for (auto byte : event.packet.payload) {
        sstr << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<unsigned>(byte);
    }
    sstr << std::dec << "\n----------------------\n";
    return sstr.str();
}

int main()
{
    // Build in-process server stack: acceptor + sink + source -> EventBus bridge.
    auto io = std::make_shared<boost::asio::io_context>();
    auto sink = std::make_shared<TcpSink>();
    ThreadPool pool(2);
    EventBus bus(pool);
    TcpSource tcpSource(bus);

    std::promise<TcpPacketEvent> busPacketPromise;
    bus.subscribe<TcpPacketEvent>([](const TcpPacketEvent& event) {
        logInfo(build_message(event));
    });

    std::thread ioThread([&io]() {
        io->run();
    });


    tcp::socket clientSocket(*io);
    boost::system::error_code ec;
    auto server_ip = boost::asio::ip::make_address_v4("192.168.2.16", ec);
    if (ec) {
        logError("Invalid IP: " + ec.message());
    }
    tcp::endpoint endpoint(server_ip, 8000);
    clientSocket.connect(endpoint, ec);
    if (ec) {
        logError("client connect failed: " + ec.message());
        io->stop();
        ioThread.join();
        return EXIT_FAILURE;
    }


    auto clientConn = std::make_shared<TcpConnection>(std::move(clientSocket));
    std::promise<TcpPacket> clientPacketPromise;
    auto clientPacketFuture = clientPacketPromise.get_future();
    clientConn->setPacketHandler([&clientPacketPromise](const TcpPacket& packet) {
        clientPacketPromise.set_value(packet);
    });
    clientConn->start();
    std::mutex mx;
    std::condition_variable cv;
    std::unique_lock<std::mutex> lock(mx);
    cv.wait(lock);
    return EXIT_SUCCESS;
}