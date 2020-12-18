#include "timebasesupervisor.h"
#include "event.h"
#include "log.h"

namespace MuonPi {

TimeBaseSupervisor::TimeBaseSupervisor(std::chrono::system_clock::duration sample_time)
    : m_sample_time { sample_time }
{}

void TimeBaseSupervisor::process_event(const Event &event)
{

    if (event.start() < m_start) {
        m_start = event.start();
    } else if (event.start() > m_end) {
        m_end = event.start();
    }
}

auto TimeBaseSupervisor::current() -> std::chrono::system_clock::duration
{
    if ((std::chrono::system_clock::now() - m_sample_start) < m_sample_time) {
        return m_current;
    }

    m_current = std::chrono::nanoseconds{m_end - m_start};

    m_start += 10000000000000;
    m_end = -1000000000;
    m_sample_start = std::chrono::system_clock::now();
    if (m_current < s_minimum) {
        m_current = s_minimum;
    }
    return m_current;
}
}
