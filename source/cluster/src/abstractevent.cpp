#include "abstractevent.h"

namespace MuonPi {

AbstractEvent::AbstractEvent(std::size_t hash, std::uint64_t id, std::chrono::steady_clock::time_point event_time) noexcept
    : m_hash { hash }
    , m_id { id}
    , m_time { event_time }
{}

AbstractEvent::~AbstractEvent() noexcept = default;

auto AbstractEvent::time() const noexcept -> std::chrono::steady_clock::time_point
{
    return m_time;
}

auto AbstractEvent::id() const noexcept -> std::uint64_t
{
    return m_id;
}

auto AbstractEvent::hash() const noexcept -> std::size_t
{
    return m_hash;
}


}
