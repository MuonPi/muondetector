#include "eventconstructor.h"
#include "event.h"
#include "combinedevent.h"
#include "criterion.h"

namespace MuonPi {

EventConstructor::EventConstructor(std::unique_ptr<Event> event, std::shared_ptr<Criterion> criterion)
    : m_event { std::make_unique<CombinedEvent>(event->id(), std::move(event)) }
    , m_criterion { criterion }
{
}

EventConstructor::~EventConstructor() = default;

auto EventConstructor::add_event(std::unique_ptr<Event> event) -> bool
{
    std::unique_ptr<Event> evt { std::make_unique<Event>(std::move(m_event)) };

    const float criterion { m_criterion->criterion(evt, event) };

    if (criterion < m_criterion->maximum_false()) {
        return false;
    }
    m_event = std::make_unique<CombinedEvent>( std::move(evt) );

    m_event->add_event(std::move(event));

    if (criterion < m_criterion->minimum_true()) {
        m_event->mark_contested();
    }
    return true;
}

void EventConstructor::set_timeout(std::chrono::steady_clock::duration timeout)
{
    m_timeout = timeout;
}

auto EventConstructor::commit() -> std::unique_ptr<Event>
{
    auto ptr { std::make_unique<Event>(m_event.release()) };

    m_event = nullptr;

    return ptr;
}

auto EventConstructor::timed_out() const -> bool
{
    return (std::chrono::steady_clock::now() - m_start) >= m_timeout;
}

}
