#include "detectorlog.h"

#include <utility>

namespace MuonPi {
	
DetectorLog::DetectorLog(std::size_t hash)
    : m_hash { hash }
{
}

DetectorLog::DetectorLog() noexcept
    : m_valid { false }
{}

DetectorLog::DetectorLog(const DetectorLog& other)
    : m_hash { other.m_hash }
    , m_deadtime { other.m_deadtime }
    , m_active { other.m_active }
    , m_mean_eventrate { other.m_mean_eventrate }
    , m_mean_pulselength { other.m_mean_pulselength }
    , m_time { other.m_time }
    , m_valid { other.m_valid }
{
}

DetectorLog::DetectorLog(DetectorLog&& other)
    : m_hash { std::move(other.m_hash) }
    , m_deadtime { std::move(other.m_deadtime) }
    , m_active { std::move(other.m_active) }
    , m_mean_eventrate { std::move(other.m_mean_eventrate) }
    , m_mean_pulselength { std::move(other.m_mean_pulselength) }
    , m_time { std::move(other.m_time) }
    , m_valid { std::move(other.m_valid) }
{
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
