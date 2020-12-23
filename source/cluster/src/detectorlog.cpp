#include "detectorlog.h"

#include <utility>

namespace MuonPi {
	
DetectorLog::DetectorLog(std::size_t hash, UserInfo user_info, Data data)
    : m_hash { hash }
    , m_data { data }
    , m_userinfo { user_info }
    , m_valid { true }
{
}

DetectorLog::DetectorLog() noexcept
    : m_valid { false }
{}

DetectorLog::DetectorLog(const DetectorLog& other)
    : m_hash { other.m_hash }
    , m_time { other.m_time }
    , m_data { other.m_data }
    , m_userinfo { other.m_userinfo }
    , m_valid { other.m_valid }
{
}

DetectorLog::DetectorLog(DetectorLog&& other)
    : m_hash { std::move(other.m_hash) }
    , m_time { std::move(other.m_time) }
    , m_data { std::move(other.m_data) }
    , m_userinfo { std::move(other.m_userinfo) }
    , m_valid { std::move(other.m_valid) }
{
}

auto DetectorLog::data() -> Data
{
    return m_data;
}

auto DetectorLog::user_info() const -> UserInfo
{
	return m_userinfo;
}

auto DetectorLog::hash() const noexcept -> std::size_t
{
    return m_hash;
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