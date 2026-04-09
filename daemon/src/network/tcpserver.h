#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "tcpconnection.h"
#include "sinks/tcp_sink.h"
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <chrono>
#include <cstdint>


class TcpSink;
class TcpServer
{
public:
    using ConnectionHandler = std::function<void(const std::shared_ptr<TcpConnection>&)>;
    using SessionId = std::uint64_t;

    struct SessionState {
        std::shared_ptr<TcpConnection> connection;
        std::chrono::steady_clock::time_point connectedAt;
        std::chrono::steady_clock::time_point lastHeartbeatSentAt;
        std::uint32_t heartbeatCount{0};
    };

    TcpServer(std::shared_ptr<boost::asio::io_context> io, std::uint16_t port, std::shared_ptr<TcpSink> sink);
    std::uint16_t port() const;
    void addConnectionHandler(ConnectionHandler handler);
    void heartbeatAndCleanup(std::chrono::steady_clock::duration maxIdle);

private:
    void do_accept();
    void removeSession(SessionId id);

    std::shared_ptr<boost::asio::io_context> io_;
    tcp::acceptor acceptor_;
    std::shared_ptr<TcpSink> sink_;
    std::unordered_map<SessionId, SessionState> sessions_;
    std::vector<ConnectionHandler> connectionHandlers_{};
    SessionId nextSessionId_{1};
};

#endif // TCPSERVER_H
