#include "singleevent.h"

namespace MuonPi {

SingleEvent::SingleEvent(std::uint64_t id, std::chrono::steady_clock::time_point time) noexcept
    : AbstractEvent{id, time}
{}

auto SingleEvent::site_id() const -> std::string
{
    return m_site_id;
}

auto SingleEvent::username() const -> std::string
{
    return m_username;
}

}
