#include "detectorlog.h"

#include <utility>

namespace MuonPi {

DetectorInfo::DetectorInfo(std::size_t hash, Location location)
    : m_hash { hash }
    , m_location { location }
{
}

DetectorInfo::DetectorInfo() noexcept
    : m_valid { false }
{}

DetectorInfo::DetectorInfo(const DetectorInfo& other)
    : m_hash { other.m_hash }
    , m_location { other.m_location }
    , m_time { other.m_time }
    , m_valid { other.m_valid }
{
}

DetectorInfo::DetectorInfo(DetectorInfo&& other)
    : m_hash { std::move(other.m_hash) }
    , m_location { std::move(other.m_location) }
    , m_time { std::move(other.m_time) }
    , m_valid { std::move(other.m_valid) }
{
}

auto DetectorInfo::hash() const noexcept -> std::size_t
{
    return m_hash;
}


auto DetectorInfo::location() const -> Location
{
    return m_location;
}


auto DetectorInfo::time() const -> std::chrono::system_clock::time_point
{
    return m_time;
}

auto DetectorInfo::valid() const -> bool
{
    return m_valid;
}
}
