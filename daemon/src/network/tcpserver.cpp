#include "network/tcpserver.h"

#include "core/event_bus.h"
#include "data/events/server_conn_count_event.h"
#include "sinks/tcp_sink.h"
#include "tcpmessage_keys.h"

using boost::asio::ip::tcp;

TcpServer::TcpServer(std::shared_ptr<boost::asio::io_context> io, std::uint16_t port,
                     std::shared_ptr<TcpSink> sink, EventBus* bus)
    : io_(io), acceptor_(*io_, tcp::endpoint(tcp::v4(), port)), sink_(sink), bus_{bus} {
    do_accept();
}

std::uint16_t TcpServer::port() const {
    return acceptor_.local_endpoint().port();
}

void TcpServer::addConnectionHandler(ConnectionHandler handler) {
    connectionHandlers_.push_back(std::move(handler));
}

void TcpServer::removeSession(SessionId id) {
    auto it = sessions_.find(id);
    if (it == sessions_.end()) {
        return;
    }

    if (it->second.connection) {
        sink_->removeConnection(it->second.connection);
    }
    sessions_.erase(it);
}

void TcpServer::heartbeatAndCleanup(std::chrono::steady_clock::duration maxIdle) {
    boost::asio::post(*io_, [this, maxIdle]() {
        const auto now = std::chrono::steady_clock::now();
        const std::vector<std::uint8_t> heartbeatPayload{};

        bool removedConnections{false};
        for (auto it = sessions_.begin(); it != sessions_.end();) {
            auto& state = it->second;
            auto& conn = state.connection;
            const bool invalid = !conn || !conn->isOpen();
            const bool stale = !invalid && ((now - conn->lastReceivedAt()) > maxIdle);
            if (invalid || stale) {
                if (conn) {
                    conn->stop();
                    sink_->removeConnection(conn);
                }
                it = sessions_.erase(it);
                removedConnections = true;
                continue;
            }

            conn->sendPacket(static_cast<std::uint16_t>(TCP_MSG_KEY::MSG_PING), heartbeatPayload);
            state.lastHeartbeatSentAt = now;
            state.heartbeatCount++;
            ++it;
        }
        if (removedConnections && bus_ != nullptr) {
            bus_->publish(
                ServerConnCountEvent{.n_sessions = sessions_.size(), .newlyConnected = false});
        }

        sink_->pruneDisconnected();
    });
}

void TcpServer::do_accept() {
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
        if (!ec) {
            const SessionId sessionId = nextSessionId_++;
            auto conn = std::make_shared<TcpConnection>(std::move(socket));
            conn->setDisconnectHandler(
                [this, sessionId](const boost::system::error_code&) { removeSession(sessionId); });

            const auto now = std::chrono::steady_clock::now();
            sessions_.emplace(sessionId, SessionState{conn, now, now, 0});
            sink_->addConnection(conn);
            for (auto& handler : connectionHandlers_) {
                handler(conn);
            }
            conn->start();
            if (bus_ != nullptr) {
                bus_->publish(
                    ServerConnCountEvent{.n_sessions = sessions_.size(), .newlyConnected = true});
            }
        }

        do_accept();
    });
}
