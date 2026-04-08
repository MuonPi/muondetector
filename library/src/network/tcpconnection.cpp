#include "tcpconnection.h"
#include <algorithm>
#include <utility>

namespace {
constexpr std::uint32_t kMagic = 0x4D554F4Eu; // "MUON"
constexpr std::size_t kHeaderSize = 12;
constexpr std::size_t kChecksumSize = 4;
constexpr std::size_t kFrameOverhead = kHeaderSize + kChecksumSize;
}


TcpConnection::TcpConnection(tcp::socket socket)
    : socket_(std::move(socket))
    , lastReceivedNanos_(nowNanos())
{
    receiveBuffer_.reserve(8192);
}


void TcpConnection::start()
{
    do_read();
}

void TcpConnection::stop()
{
    auto self = shared_from_this();
    boost::asio::post(socket_.get_executor(), [this, self]() {
        boost::system::error_code ignored;
        socket_.close(ignored);
    });
}

void TcpConnection::sendPacket(std::uint16_t key, const std::vector<std::uint8_t>& payload)
{
    auto self = shared_from_this();
    boost::asio::post(socket_.get_executor(), [this, self, key, payload]() {
        bool writing = !writeQueue_.empty();
        writeQueue_.push_back(buildFrame(key, payload));
        if (!writing) {
            do_write();
        }
    });
}

void TcpConnection::sendPacket(std::uint16_t key, std::vector<std::uint8_t>&& payload)
{
    auto self = shared_from_this();
    boost::asio::post(socket_.get_executor(), [this, self, key, payload = std::move(payload)]() mutable {
        bool writing = !writeQueue_.empty();
        writeQueue_.push_back(buildFrame(key, payload));
        if (!writing) {
            do_write();
        }
    });
}

void TcpConnection::setPacketHandler(PacketHandler handler)
{
    packetHandler_ = std::move(handler);
}

void TcpConnection::setDisconnectHandler(DisconnectHandler handler)
{
    disconnectHandler_ = std::move(handler);
}

bool TcpConnection::isOpen() const
{
    return socket_.is_open();
}

auto TcpConnection::lastReceivedAt() const -> clock_type::time_point
{
    return clock_type::time_point(std::chrono::nanoseconds(lastReceivedNanos_.load()));
}

auto TcpConnection::socket() const -> const tcp::socket&
{
    return socket_;
}

void TcpConnection::consumeIncomingBytes(std::size_t n)
{
    receiveBuffer_.insert(receiveBuffer_.end(), readBuffer_.begin(), readBuffer_.begin() + n);
}

void TcpConnection::closeWithError(const boost::system::error_code& ec)
{
    if (socket_.is_open()) {
        boost::system::error_code ignored;
        socket_.close(ignored);
    }

    if (disconnectHandler_) {
        disconnectHandler_(ec);
    }
}

void TcpConnection::do_read()
{
    auto self = shared_from_this();

    socket_.async_read_some(
        boost::asio::buffer(readBuffer_),
        [this, self](boost::system::error_code ec, std::size_t bytesRead)
        {
            if (ec) {
                closeWithError(ec);
                return;
            }

            consumeIncomingBytes(bytesRead);

            while (tryParsePacket()) {
            }

            do_read();
        });
}

auto TcpConnection::tryParsePacket() -> bool
{
    if (receiveBuffer_.size() < kHeaderSize) {
        return false;
    }

    const auto* header = receiveBuffer_.data();
    const std::uint32_t magic = readU32(header);
    const std::uint16_t version = readU16(header + 4);
    const std::uint16_t key = readU16(header + 6);
    const std::uint32_t payloadSize = readU32(header + 8);

    if (magic != kMagic) {
        closeWithError(boost::asio::error::make_error_code(boost::asio::error::fault));
        return false;
    }

    if (version != kProtocolVersion) {
        closeWithError(boost::asio::error::make_error_code(boost::asio::error::operation_not_supported));
        return false;
    }

    if (payloadSize > kMaxPayloadSize) {
        closeWithError(boost::asio::error::make_error_code(boost::asio::error::message_size));
        return false;
    }

    const std::size_t frameSize = kFrameOverhead + payloadSize;
    if (receiveBuffer_.size() < frameSize) {
        return false;
    }

    std::vector<std::uint8_t> payload(payloadSize);
    if (payloadSize > 0) {
        std::copy_n(receiveBuffer_.begin() + kHeaderSize, payloadSize, payload.begin());
    }

    const std::uint32_t checksumValue = readU32(receiveBuffer_.data() + kHeaderSize + payloadSize);
    if (checksumValue != checksum(payload)) {
        closeWithError(boost::asio::error::make_error_code(boost::asio::error::bad_descriptor));
        return false;
    }

    receiveBuffer_.erase(receiveBuffer_.begin(), receiveBuffer_.begin() + frameSize);

    if (packetHandler_) {
        packetHandler_(TcpPacket{ key, std::move(payload) });
    }
    lastReceivedNanos_.store(nowNanos());

    return true;
}

void TcpConnection::do_write()
{
    auto self = shared_from_this();

    boost::asio::async_write(socket_,
        boost::asio::buffer(writeQueue_.front()),
        [this, self](boost::system::error_code ec, std::size_t)
        {
            if (ec) {
                closeWithError(ec);
                return;
            }

            writeQueue_.pop_front();
            if (!writeQueue_.empty())
                do_write();
        });
}

auto TcpConnection::checksum(const std::vector<std::uint8_t>& payload) -> std::uint32_t
{
    std::uint32_t hash = 2166136261u;
    for (auto b : payload) {
        hash ^= b;
        hash *= 16777619u;
    }
    return hash;
}

auto TcpConnection::buildFrame(std::uint16_t key, const std::vector<std::uint8_t>& payload) -> std::vector<std::uint8_t>
{
    const std::size_t payloadSize = payload.size();
    std::vector<std::uint8_t> frame(kFrameOverhead + payloadSize);
    auto* p = frame.data();

    writeU32(p, kMagic);
    writeU16(p + 4, kProtocolVersion);
    writeU16(p + 6, key);
    writeU32(p + 8, static_cast<std::uint32_t>(payloadSize));

    if (payloadSize > 0) {
        std::copy(payload.begin(), payload.end(), p + kHeaderSize);
    }

    writeU32(p + kHeaderSize + payloadSize, checksum(payload));
    return frame;
}

auto TcpConnection::readU16(const std::uint8_t* p) -> std::uint16_t
{
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(p[0]) << 8) |
                                      static_cast<std::uint16_t>(p[1]));
}

auto TcpConnection::readU32(const std::uint8_t* p) -> std::uint32_t
{
    return (static_cast<std::uint32_t>(p[0]) << 24) |
           (static_cast<std::uint32_t>(p[1]) << 16) |
           (static_cast<std::uint32_t>(p[2]) << 8) |
           static_cast<std::uint32_t>(p[3]);
}

void TcpConnection::writeU16(std::uint8_t* p, std::uint16_t v)
{
    p[0] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
    p[1] = static_cast<std::uint8_t>(v & 0xFF);
}

void TcpConnection::writeU32(std::uint8_t* p, std::uint32_t v)
{
    p[0] = static_cast<std::uint8_t>((v >> 24) & 0xFF);
    p[1] = static_cast<std::uint8_t>((v >> 16) & 0xFF);
    p[2] = static_cast<std::uint8_t>((v >> 8) & 0xFF);
    p[3] = static_cast<std::uint8_t>(v & 0xFF);
}

auto TcpConnection::nowNanos() -> std::uint64_t
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            clock_type::now().time_since_epoch())
            .count());
}
