#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <boost/asio.hpp>
#include <array>
#include <deque>
#include <memory>
#include <iostream>

using boost::asio::ip::tcp;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    explicit TcpConnection(tcp::socket socket);

    void start();

    void send(const std::vector<uint8_t>& msg);

private:
    void do_write();
    void do_read();

    tcp::socket socket_;
    std::array<char, 2048> buffer_;
    std::deque<std::vector<uint8_t>> writeQueue_;
};

#endif // TCP_CONNECTION_H