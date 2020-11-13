#include "abstracteventsink.h"
#include "abstractevent.h"

namespace MuonPi {

AbstractEventSink::AbstractEventSink()
{
}

AbstractEventSink::~AbstractEventSink()
{

}

auto AbstractEventSink::next_event() -> std::future<std::unique_ptr<AbstractEvent>>
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

void AbstractEventSink::push_event(std::unique_ptr<AbstractEvent> /*event*/)
{

}

void AbstractEventSink::run()
{

}

}
