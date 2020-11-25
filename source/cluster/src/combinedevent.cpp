#include "combinedevent.h"

namespace MuonPi {


CombinedEvent::CombinedEvent(std::uint64_t id, std::unique_ptr<AbstractEvent> event) noexcept
    : AbstractEvent{event->hash(), id, event->time()}
{}

CombinedEvent::~CombinedEvent() noexcept = default;

auto CombinedEvent::add_event(std::unique_ptr<AbstractEvent> /*event*/) noexcept -> bool
{
    return false;
}

auto CombinedEvent::n() const noexcept -> std::size_t
{
    return 0;
}

auto CombinedEvent::events() -> std::vector<std::unique_ptr<AbstractEvent>>
{
    m_valid = false;
    return std::move(m_events);
}
}
