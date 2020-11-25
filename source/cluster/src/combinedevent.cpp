#include "combinedevent.h"


namespace MuonPi {


CombinedEvent::CombinedEvent(std::uint64_t id, std::unique_ptr<AbstractEvent> event) noexcept
    : AbstractEvent{event->hash(), id}
{}

CombinedEvent::~CombinedEvent() noexcept = default;

auto CombinedEvent::add_event(std::unique_ptr<AbstractEvent> event) noexcept -> bool
{
    if (event == nullptr) {
        return false;
    }
    m_events.push_back(std::move(event));
    return true;
}

auto CombinedEvent::n() const noexcept -> std::size_t
{
    return m_events.size();
}

auto CombinedEvent::events() -> const std::vector<std::unique_ptr<AbstractEvent> >&
{
    return m_events;
}

auto CombinedEvent::time() const noexcept -> std::vector<std::chrono::steady_clock::time_point>
{
    std::vector<std::chrono::steady_clock::time_point> vector {};
    for (const auto& event: m_events) {
        auto time { std::move(event->time()) };
        vector.insert(vector.end(), time.begin(), time.end());
    }
    return std::move(vector);
}
}
