#include "singleevent.h"

namespace MuonPi {

SingleEvent::SingleEvent(std::size_t hash, std::uint64_t id, std::chrono::steady_clock::time_point time, std::chrono::steady_clock::duration duration) noexcept
    : AbstractEvent{hash, id, time}
    , m_duration { duration }
{}

SingleEvent::SingleEvent(std::size_t hash, std::uint64_t id, std::chrono::steady_clock::time_point rising, std::chrono::steady_clock::time_point falling) noexcept
    : AbstractEvent{hash, id, rising}
    , m_duration { falling - rising }
{}

SingleEvent::~SingleEvent() noexcept = default;


auto SingleEvent::duration() const -> std::chrono::steady_clock::duration
{
    return m_duration;
}

}
