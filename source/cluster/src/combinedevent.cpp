#include "combinedevent.h"


namespace MuonPi {


CombinedEvent::CombinedEvent(std::uint64_t id, std::unique_ptr<Event> event) noexcept
    : Event{event->hash(), id, event->start(), event->start()}
{}

CombinedEvent::~CombinedEvent() noexcept = default;

void CombinedEvent::add_event(std::unique_ptr<Event> event) noexcept
{
    if (event->start() <= start()) {
        m_start = event->start();
    }
    if (event->start() >= end()) {
        m_end = event->start();
    }
    m_events.push_back(std::move(event));
    m_n++;
}

void CombinedEvent::mark_contested()
{
    m_contested = true;
}

auto CombinedEvent::contested() const -> bool
{
    return m_contested;
}

auto CombinedEvent::events_ref() const -> const std::vector<std::unique_ptr<Event>>&
{
    return m_events;
}

auto CombinedEvent::events() -> std::vector<std::unique_ptr<Event>>
{
    return std::move(m_events);
}

}
