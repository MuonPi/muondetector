#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "tcpconnection.h"
#include "sinks/tcp_sink.h"
#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include <iostream>


class TcpSink;
class TcpServer
{
public:
    TcpServer(std::shared_ptr<boost::asio::io_context> io, std::uint16_t port, std::shared_ptr<TcpSink> sink);

private:
    void do_accept();

    std::shared_ptr<boost::asio::io_context> io_;
    tcp::acceptor acceptor_;
    std::shared_ptr<TcpSink> sink_;
    std::vector<std::shared_ptr<TcpConnection>> sessions_;
};

#endif // TCPSERVER_H