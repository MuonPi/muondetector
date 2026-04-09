#include "network/tcpserver.h"
#include "sinks/tcp_sink.h"

using boost::asio::ip::tcp;

TcpServer::TcpServer(std::shared_ptr<boost::asio::io_context> io, std::uint16_t port, std::shared_ptr<TcpSink> sink)
    : io_(io), acceptor_(*io_, tcp::endpoint(tcp::v4(), port)), sink_(sink)
{
    do_accept();
}

void TcpServer::do_accept()
{
    acceptor_.async_accept([this](boost::system::error_code ec, tcp::socket socket) {
        if (!ec)
        {
            auto conn = std::make_shared<TcpConnection>(std::move(socket));
            conn->start();

            sink_->addConnection(conn);
        }

        do_accept();
    });
}
