#include "abstracteventsource.h"
#include "abstractevent.h"

namespace MuonPi {

AbstractEventSource::AbstractEventSource()
{
}

AbstractEventSource::~AbstractEventSource()
{

}

auto AbstractEventSource::next_event() -> std::future<std::unique_ptr<AbstractEvent>>
{
    return {};
}

void AbstractEventSource::stop()
{

}

auto AbstractEventSource::step() -> bool
{
    return false;
}

void AbstractEventSource::push_event(std::unique_ptr<AbstractEvent> /*event*/)
{

}

void AbstractEventSource::run()
{

}

}
