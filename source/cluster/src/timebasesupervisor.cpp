#include "timebasesupervisor.h"
#include "event.h"

namespace MuonPi {

TimeBaseSupervisor::TimeBaseSupervisor(std::chrono::steady_clock::duration sample_time)
    : m_sample_time { sample_time }
{}

void TimeBaseSupervisor::process_event(const Event &event)
{
    if (event.start() < m_earliest) {
        m_earliest = event.start();
    }
    if (event.end() > m_latest) {
        m_latest = event.start();
    }
}

auto TimeBaseSupervisor::current() -> std::chrono::steady_clock::duration
{
    if ((std::chrono::steady_clock::now() - m_start) < m_sample_time) {
        return m_current;
    }
    m_current = m_latest - m_earliest;
    m_latest = std::chrono::steady_clock::now() - std::chrono::hours{24};
    m_earliest = std::chrono::steady_clock::now() + std::chrono::hours{24};
    m_start = std::chrono::steady_clock::now();
    return m_current;
}
}
