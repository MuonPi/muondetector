#include "logmessage.h"

namespace MuonPi {

LogMessage::LogMessage(std::size_t hash, Location location)
    : m_hash { hash }
    , m_location { location }
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
}
