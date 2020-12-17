#include "detectorlog.h"

#include <utility>

namespace MuonPi {

DetectorLog::DetectorLog(std::size_t hash, Location location)
    : m_hash { hash }
    , m_location { location }
{
}

DetectorLog::DetectorLog() noexcept
    : m_valid { false }
{}

DetectorLog::DetectorLog(const DetectorLog& other)
    : m_hash { other.m_hash }
    , m_location { other.m_location }
    , m_time { other.m_time }
    , m_valid { other.m_valid }
{
}

DetectorLog::DetectorLog(DetectorLog&& other)
    : m_hash { std::move(other.m_hash) }
    , m_location { std::move(other.m_location) }
    , m_time { std::move(other.m_time) }
    , m_valid { std::move(other.m_valid) }
{
}

auto DetectorLog::hash() const noexcept -> std::size_t
{
    return m_hash;
}


auto DetectorLog::location() const -> Location
{
    return m_location;
}


auto DetectorLog::time() const -> std::chrono::system_clock::time_point
{
    return m_time;
}

auto DetectorLog::valid() const -> bool
{
    return m_valid;
}
}
