#include "logmessage.h"

#include <utility>

namespace MuonPi {

LogMessage::LogMessage(std::size_t hash, Location location)
    : m_hash { hash }
    , m_location { location }
{
}

LogMessage::LogMessage() noexcept
    : m_valid { false }
{}

LogMessage::LogMessage(const LogMessage& other)
    : m_hash { other.m_hash }
    , m_location { other.m_location }
    , m_time { other.m_time }
    , m_valid { other.m_valid }
{
}

LogMessage::LogMessage(LogMessage&& other)
    : m_hash { std::move(other.m_hash) }
    , m_location { std::move(other.m_location) }
    , m_time { std::move(other.m_time) }
    , m_valid { std::move(other.m_valid) }
{
}

auto LogMessage::hash() const noexcept -> std::size_t
{
    return m_hash;
}


auto LogMessage::location() const -> Location
{
    return m_location;
}


auto LogMessage::time() const -> std::chrono::system_clock::time_point
{
    return m_time;
}

auto LogMessage::valid() const -> bool
{
    return m_valid;
}
}
