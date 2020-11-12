#include "abstractevent.h"

namespace MuonPi {

AbstractEvent::AbstractEvent(uint64_t id, std::chrono::steady_clock::time_point event_time) noexcept
    : m_id { id}
    , m_time { event_time }
{}

auto AbstractEvent::time() const -> std::chrono::steady_clock::time_point
{
    return m_time;
}

auto AbstractEvent::id() const -> std::uint64_t
{
    return m_id;
}

}
