#include "abstracteventsink.h"
#include "event.h"

namespace MuonPi {

AbstractEventSink::~AbstractEventSink()
{
    ThreadRunner::~ThreadRunner();
}

auto AbstractEventSink::next_event() -> std::future<std::unique_ptr<Event>>
{
    if (m_has_event_promise) {
        return {};
    }

    if (m_event_queue.empty()) {
        m_event_promise = std::promise<std::unique_ptr<Event>>();
        return m_event_promise.get_future();
    }

    return std::async(std::launch::deferred, [this]{
        auto evt {std::move(m_event_queue.front())};
        m_event_queue.pop();
        return evt;});
}

void AbstractEventSink::push_event(std::unique_ptr<Event> event)
{
    if (m_has_event_promise) {
        m_event_promise.set_value(std::move(event));
        m_has_event_promise = false;
        return;
    }
    m_event_queue.push(std::move(event));
}

}
