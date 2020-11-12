#include "singleevent.h"

namespace MuonPi {

SingleEvent::SingleEvent(std::uint64_t id, std::chrono::steady_clock::time_point time) noexcept
    : AbstractEvent{id, time}
{}

SingleEvent::~SingleEvent() noexcept = default;

auto SingleEvent::site_id() const noexcept -> std::string
{
    return m_site_id;
}

auto SingleEvent::username() const noexcept -> std::string
{
    return m_username;
}

}
