#include "abstracteventsource.h"
#include "event.h"

namespace MuonPi {

AbstractEventSource::AbstractEventSource()
{
}

AbstractEventSource::~AbstractEventSource()
{

}

auto AbstractEventSource::next_event() -> std::future<std::unique_ptr<Event>>
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

void AbstractEventSource::push_event(std::unique_ptr<Event> /*event*/)
{

}

void AbstractEventSource::run()
{

}

}
