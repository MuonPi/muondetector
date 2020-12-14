#include "eventconstructor.h"
#include "criterion.h"
#include "log.h"

namespace MuonPi {

EventConstructor::EventConstructor(Event event, std::shared_ptr<Criterion> criterion, std::chrono::system_clock::duration timeout)
    : m_event { event.id(), std::move(event) }
    , m_criterion { criterion }
    , m_timeout { timeout }
{
}

EventConstructor::~EventConstructor() = default;

void EventConstructor::add_event(Event event, bool contested)
{
    m_event.add_event(std::move(event));

    if (contested) {
        m_event.mark_contested();
    }
}

auto EventConstructor::event_matches(const Event& event) -> EventConstructor::Type
{
    const float criterion { m_criterion->criterion(m_event, event) };

    if (criterion < m_criterion->maximum_false()) {
        return Type::NoMatch;
    }

    if (criterion < m_criterion->minimum_true()) {
        return Type::Contested;
    }

    return Type::Match;
}

void EventConstructor::set_timeout(std::chrono::system_clock::duration timeout)
{
    if (timeout <= m_timeout) {
        return;
    }
    m_timeout = timeout;
}

auto EventConstructor::commit() -> Event
{
   return std::move(m_event);
}

auto EventConstructor::timed_out() const -> bool
{
    return (std::chrono::system_clock::now() - m_start) >= m_timeout;
}

}
