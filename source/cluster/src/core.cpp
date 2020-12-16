#include "core.h"
#include "log.h"

#include "abstractsink.h"
#include "abstractsource.h"
#include "criterion.h"
#include "eventconstructor.h"
#include "event.h"
#include "logmessage.h"
#include "timebasesupervisor.h"
#include "detectortracker.h"
#include "log.h"

namespace MuonPi {

Core::Core(std::unique_ptr<AbstractSink<Event>> event_sink, std::unique_ptr<AbstractSource<Event>> event_source, std::unique_ptr<AbstractDetectorTracker> detector_tracker, StateSupervisor& supervisor)
    : ThreadRunner{"Core"}
    , m_event_sink { std::move(event_sink) }
    , m_event_source { std::move(event_source) }
    , m_detector_tracker { std::move(detector_tracker) }
    , m_supervisor { supervisor }
{
    start();
}

auto Core::supervisor() -> StateSupervisor&
{
    return m_supervisor;
}

auto Core::step() -> int
{
    if (m_event_sink->state() <= ThreadRunner::State::Stopped) {
        Log::error()<<"The event sink stopped.";
        return -1;
    }
    if (m_event_source->state() <= ThreadRunner::State::Stopped) {
        Log::error()<<"The event source stopped.";
        return -1;
    }
    if (m_detector_tracker->state() <= ThreadRunner::State::Stopped) {
        Log::error()<<"The Detector tracker stopped.";
        return -1;
    }
    if (m_supervisor.step() != 0) {
        return -1;
    }
    {
        using namespace std::chrono;
        m_timeout = milliseconds{static_cast<long>(static_cast<float>(duration_cast<milliseconds>(m_time_base_supervisor->current()).count()) * m_detector_tracker->factor() * m_scale)};
        m_supervisor.time_status(duration_cast<milliseconds>(m_timeout));
    }
    // +++ Send finished constructors off to the event sink
    for (auto& [id, constructor]: m_constructors) {
        constructor->set_timeout(m_timeout);
        if (constructor->timed_out()) {
            m_delete_constructors.push(id);

            m_event_sink->push_item(constructor->commit());
        }
    }

    // +++ Delete finished constructors
    while (!m_delete_constructors.empty()) {
        m_constructors.erase(m_delete_constructors.front());
        m_delete_constructors.pop();
    }
    // --- Delete finished constructors

    // --- Send finished constructors off to the event sink

    std::size_t before { m_event_source->size() };
    // +++ handle incoming events, maximum 10 at a time to prevent blocking

    std::size_t i { 0 };
    while (m_event_source->has_items() && (i < 100)) {
        process(m_event_source->next_item());
        i++;
    }
    // --- handle incoming events, maximum 10 at a time to prevent blocking

    // +++ decrease the timeout to level the load if there is a large backlog
    if ((before != 0) && (before > i)) {
        m_scale = (static_cast<float>(i) / static_cast<float>(before));
        Log::warning()<<"Scaling timeout: " + std::to_string(m_scale);
    } else {
        m_scale = 1.0f;
    }
    // --- decrease the timeout to level the load if there is a large backlog

    std::this_thread::sleep_for( std::chrono::microseconds{500} );
    return 0;
}

auto Core::post_run() -> int
{
    m_detector_tracker->stop();
    m_event_sink->stop();
    m_event_source->stop();
    m_detector_tracker->join();
    m_event_sink->join();
    m_event_source->join();
    return m_detector_tracker->wait() + m_event_sink->wait() + m_event_source->wait();
}

void Core::process(Event event)
{
    m_time_base_supervisor->process_event(event);

    if (!m_detector_tracker->accept(event)) {
        return;
    }


    std::queue<std::uint64_t> matches {};
    for (auto& [id, constructor]: m_constructors) {
        auto result { constructor->event_matches(event) };
        if ( EventConstructor::Type::NoMatch != result) {
            matches.push(id);
        }
    }
    std::uint64_t event_id { event.id() };

    // +++ Event matches exactly one existing constructor
    if (matches.size() == 1) {
        m_constructors[matches.front()]->add_event(std::move(event));
        matches.pop();
        return;
    }
    // --- Event matches exactly one existing constructor

    // +++ Event matches either no, or more than one constructor
    std::unique_ptr<EventConstructor> constructor { std::make_unique<EventConstructor>(std::move(event), m_criterion, m_timeout) };

    // +++ Event matches more than one constructor
    // Combines all contesting constructors into one contesting coincience
    while (!matches.empty()) {
        constructor->add_event(m_constructors[matches.front()]->commit());
        m_constructors.erase(matches.front());
        matches.pop();
    }
    // --- Event matches more than one constructor
    m_constructors[event_id] = std::move(constructor);
    // --- Event matches either no, or more than one constructor
}

}
