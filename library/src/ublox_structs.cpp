#include "ublox_structs.h"
#include <algorithm>

UbxMessage::UbxMessage(std::uint16_t msg_id, const std::string a_payload) noexcept
    : m_full_id(msg_id)
    , m_payload(a_payload)
{
}

auto UbxMessage::full_id() const -> std::uint16_t
{
    return m_full_id;
}

auto UbxMessage::payload() const -> const std::string&
{
    return m_payload;
}

auto UbxMessage::class_id() const -> std::uint8_t
{
    return (m_full_id >> 8) & 0xff;
}

auto UbxMessage::message_id() const -> std::uint8_t
{
    return m_full_id & 0xff;
}

auto UbxMessage::raw_message_string() const -> std::string
{
    std::string raw_data_string { static_cast<char>(0xb5), static_cast<char>(0x62),
        static_cast<char>(class_id()), static_cast<char>(message_id()),
        static_cast<char>(m_payload.size() & 0xff),
        static_cast<char>((static_cast<std::uint16_t>(m_payload.size() & 0xff00) >> 8)) };
    raw_data_string += m_payload;
    // calc Fletcher checksum, ignore the message header (b5 62)
    auto chksum { check_sum(raw_data_string.substr(2)) };
    raw_data_string += static_cast<unsigned char>(chksum & 0xff);
    raw_data_string += static_cast<unsigned char>(chksum >> 8);
    return raw_data_string;
}

auto UbxMessage::check_sum() const -> std::uint16_t
{
    std::string raw_data_string {
        static_cast<char>(class_id()), static_cast<char>(message_id()),
        static_cast<char>(m_payload.size() & 0xff),
        static_cast<char>((m_payload.size() >> 8) & 0xff)
    };
    raw_data_string += m_payload;
    return check_sum(raw_data_string);
}

auto UbxMessage::check_sum(const std::string& data) -> std::uint16_t
{
    struct CheckSum {
        void operator()(const char& ch) { chkB += chkA += ch; }
        std::uint8_t chkA { 0 };
        std::uint8_t chkB { 0 };
    };
    CheckSum checksum { std::for_each(data.begin(), data.end(), CheckSum()) };
    return ((checksum.chkB << 8) | checksum.chkA);
}
