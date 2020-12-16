#include "timebasesupervisor.h"
#include "event.h"
#include "log.h"

namespace MuonPi {

TimeBaseSupervisor::TimeBaseSupervisor(std::chrono::system_clock::duration sample_time)
    : m_sample_time { sample_time }
{}

void TimeBaseSupervisor::process_event(const Event &event)
{
    std::int_fast64_t offset { (m_epoch - event.epoch()) * static_cast<std::int_fast64_t>(1e9) + (m_start - event.start()) };


    if (offset > 0) {
        m_start -= offset;
    } else if ((m_start - offset) > m_end) {
        m_end = m_start - offset;
    }
}

auto TimeBaseSupervisor::current() -> std::chrono::system_clock::duration
{
    if ((std::chrono::system_clock::now() - m_sample_start) < m_sample_time) {
        return m_current;
    }

    m_current = std::chrono::nanoseconds{m_end - m_start};

    m_epoch = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    m_start = 1000000000;
    m_end = -1000000000;
    m_sample_start = std::chrono::system_clock::now();
    if (m_current < s_minimum) {
        m_current = s_minimum;
    }
    return m_current;
}
}
