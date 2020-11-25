#include "eventconstructor.h"
#include "abstractevent.h"
#include "combinedevent.h"
#include "criterion.h"

namespace MuonPi {

EventConstructor::EventConstructor(std::unique_ptr<AbstractEvent> /*event*/, std::shared_ptr<Criterion> /*criterion*/)
{

}
EventConstructor::~EventConstructor()
{

}

void EventConstructor::add_event(std::unique_ptr<AbstractEvent> /*event*/)
{

}
void EventConstructor::set_timeout(std::chrono::steady_clock::duration /*timeout*/)
{

}

auto EventConstructor::event_fits(std::unique_ptr<AbstractEvent> /*event*/) -> bool
{
    return {};
}

auto EventConstructor::commit() -> std::unique_ptr<AbstractEvent>
{
    return {};
}

auto EventConstructor::timed_out() const -> bool
{
    return {};
}

}
