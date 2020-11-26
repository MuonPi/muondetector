#include "abstracteventsink.h"
#include "event.h"

namespace MuonPi {

AbstractEventSink::AbstractEventSink()
{
}

AbstractEventSink::~AbstractEventSink()
{

}

auto AbstractEventSink::next_event() -> std::future<std::unique_ptr<Event>>
{
    return {};
}

void AbstractEventSink::stop()
{

}

auto AbstractEventSink::step() -> bool
{
    return false;
}

void AbstractEventSink::push_event(std::unique_ptr<Event> /*event*/)
{

}

void AbstractEventSink::run()
{

}

}
