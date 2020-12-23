#include "detectorinfo.h"

#include <utility>

namespace MuonPi {

DetectorInfo::DetectorInfo(std::size_t hash, /* std::string msg_time,*/ UserInfo user_info, Location location)
    : m_hash { hash }
    /*, m_msg_time { msg_time } */
    , m_location { location }
    , m_userinfo { user_info }
{
}

DetectorInfo::DetectorInfo() noexcept
    : m_valid { false }
{}

DetectorInfo::DetectorInfo(const DetectorInfo& other)
    : m_hash { other.m_hash }
    , m_msg_time { other.m_msg_time }
    , m_location { other.m_location }
    , m_userinfo { other.m_userinfo }
    , m_time { other.m_time }
    , m_valid { other.m_valid }
{
}

DetectorInfo::DetectorInfo(DetectorInfo&& other)
    : m_hash { std::move(other.m_hash) }
    , m_msg_time { std::move(other.m_msg_time) }
    , m_location { std::move(other.m_location) }
	, m_userinfo { std::move(other.m_userinfo) }
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

auto DetectorInfo::user_info() const -> UserInfo
{
	return m_userinfo;
}

auto DetectorInfo::time() const -> std::chrono::system_clock::time_point
{
    return m_time;
}
/*
auto DetectorInfo::valid() const -> bool
{
    return m_valid;
}
*/
}
