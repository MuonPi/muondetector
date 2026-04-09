#include "tcpconnection.h"
#include <boost/asio.hpp>


TcpConnection::TcpConnection(tcp::socket socket)
    : socket_(std::move(socket)) {}


void TcpConnection::start()
{
    do_read();
}

void TcpConnection::send(const std::vector<uint8_t>& msg)
{
    auto self = shared_from_this();

    boost::asio::post(socket_.get_executor(),
        [this, self, msg]() mutable
        {
            bool writing = !writeQueue_.empty();
            writeQueue_.push_back(std::move(msg));

            if (!writing)
                do_write();
        });
}

void TcpConnection::do_read()
{
    auto self = shared_from_this();

    socket_.async_read_some(
        boost::asio::buffer(buffer_),
        [this, self](boost::system::error_code ec, std::size_t)
        {
            if (!ec)
                do_read();
        });
}

void TcpConnection::do_write()
{
    auto self = shared_from_this();

    boost::asio::async_write(socket_,
        boost::asio::buffer(writeQueue_.front()),
        [this, self](boost::system::error_code ec, std::size_t)
        {
            if (!ec)
            {
                writeQueue_.pop_front();
                if (!writeQueue_.empty())
                    do_write();
            }
        });
}