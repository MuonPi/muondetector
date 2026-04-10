#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#include <boost/asio.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <atomic>
#include <chrono>
#include <vector>

using boost::asio::ip::tcp;

struct TcpPacket
{
    std::uint16_t key;
    std::vector<std::uint8_t> payload;
};

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using PacketHandler = std::function<void(const TcpPacket&)>;
    using DisconnectHandler = std::function<void(const boost::system::error_code&)>;

    static constexpr std::uint16_t kProtocolVersion = 1;
    static constexpr std::size_t kMaxPayloadSize = 1024 * 1024; // 1 MiB
    using clock_type = std::chrono::steady_clock;

    explicit TcpConnection(tcp::socket socket);

    void start();
    void stop();

    void sendPacket(std::uint16_t key, const std::vector<std::uint8_t>& payload);
    void sendPacket(std::uint16_t key, std::vector<std::uint8_t>&& payload);

    void setPacketHandler(PacketHandler handler);
    void setDisconnectHandler(DisconnectHandler handler);
    bool isOpen() const;
    auto lastReceivedAt() const -> clock_type::time_point;

    // Getter for relevant socket informations
    auto socket() const -> const tcp::socket&;

private:
    void do_write();
    void do_read();
    void closeWithError(const boost::system::error_code& ec);
    void consumeIncomingBytes(std::size_t n);
    auto tryParsePacket() -> bool;
    auto buildFrame(std::uint16_t key, const std::vector<std::uint8_t>& payload) -> std::vector<std::uint8_t>;
    static auto checksum(const std::vector<std::uint8_t>& payload) -> std::uint32_t;
    static auto readU16(const std::uint8_t* p) -> std::uint16_t;
    static auto readU32(const std::uint8_t* p) -> std::uint32_t;
    static void writeU16(std::uint8_t* p, std::uint16_t v);
    static void writeU32(std::uint8_t* p, std::uint32_t v);
    static auto nowNanos() -> std::uint64_t;

    tcp::socket socket_;
    std::array<std::uint8_t, 4096> readBuffer_{};
    std::vector<std::uint8_t> receiveBuffer_{};
    std::deque<std::vector<std::uint8_t>> writeQueue_;
    PacketHandler packetHandler_{};
    DisconnectHandler disconnectHandler_{};
    std::atomic<std::uint64_t> lastReceivedNanos_;
};

#endif // TCP_CONNECTION_H
