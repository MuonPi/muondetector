#include "timebasesupervisor.h"
#include "event.h"

namespace MuonPi {
auto TimeBaseSupervisor::restart() -> std::chrono::steady_clock::duration
{
    m_current = m_latest - m_earliest;
    m_latest = std::chrono::steady_clock::now() - std::chrono::hours{24};
    m_earliest = std::chrono::steady_clock::now() + std::chrono::hours{24};
    return m_current;
}

void TimeBaseSupervisor::process_event(const std::unique_ptr<Event>& event)
{
    m_n++;
    if (event->start() < m_earliest) {
        m_earliest = event->start();
    }
    if (event->end() > m_latest) {
        m_latest = event->start();
    }
}

auto TimeBaseSupervisor::current() const -> std::chrono::steady_clock::duration
{
    return m_current;
}
}
