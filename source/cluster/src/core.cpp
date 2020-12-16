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

#include <cinttypes>

namespace MuonPi {

Core::Core(std::vector<std::shared_ptr<AbstractSink<Event>>> event_sinks, std::vector<std::shared_ptr<AbstractSource<Event>>> event_sources, DetectorTracker& detector_tracker, StateSupervisor& supervisor)
    : ThreadRunner{"Core"}
    , m_event_sinks { std::move(event_sinks) }
    , m_event_sources { std::move(event_sources) }
    , m_detector_tracker { (detector_tracker) }
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
    for (auto& sink: m_event_sinks) {
        if (sink->state() <= ThreadRunner::State::Stopped) {
            Log::error()<<"The event sink stopped.";
            return -1;
        }
    }
    for (auto& source: m_event_sources) {
        if (source->state() <= ThreadRunner::State::Stopped) {
            Log::error()<<"The event source stopped.";
            return -1;
        }
    }
    if (m_detector_tracker.state() <= ThreadRunner::State::Stopped) {
        Log::error()<<"The Detector tracker stopped.";
        return -1;
    }

    if (m_supervisor.step() != 0) {
        return -1;
    }

    {
        using namespace std::chrono;
        m_timeout = milliseconds{static_cast<long>(static_cast<float>(duration_cast<milliseconds>(m_time_base_supervisor->current()).count()) * m_detector_tracker.factor() * m_scale)};
        m_supervisor.time_status(duration_cast<milliseconds>(m_timeout));
    }
    // +++ Send finished constructors off to the event sink
    for (ssize_t i { static_cast<ssize_t>(m_constructors.size()) - 1 }; i >= 0; i--) {
        auto& constructor { m_constructors[static_cast<std::size_t>(i)] };
        constructor->set_timeout(m_timeout);
        if (constructor->timed_out()) {
            push_event(constructor->commit());
            m_constructors.erase(m_constructors.begin() + i);
        }
    }


    // --- Send finished constructors off to the event sink

    std::size_t before { 0 };
    // +++ handle incoming events, maximum 10 at a time to prevent blocking

    std::size_t processed { 0 };
    for (auto& source: m_event_sources) {
        std::size_t i { 0 };
        before += source->size();
        while (source->has_items() && (i < 100)) {
            process(source->next_item());
            i++;
        }
        processed += i;
    }
    // --- handle incoming events, maximum 10 at a time to prevent blocking

    // +++ decrease the timeout to level the load if there is a large backlog
    if ((before != 0) && (before > processed)) {
        m_scale = (static_cast<float>(processed) / static_cast<float>(before));
        Log::warning()<<"Scaling timeout: " + std::to_string(m_scale);
    } else {
        m_scale = 1.0f;
    }
    // --- decrease the timeout to level the load if there is a large backlog

    m_supervisor.set_queue_size(m_constructors.size());
    std::this_thread::sleep_for( std::chrono::microseconds{500} );
    return 0;
}

auto Core::post_run() -> int
{
    int result { 0 };

    for (auto& source: m_event_sources) {
        source->stop();
        result += source->wait();
    }
    for (auto& sink: m_event_sinks) {
        sink->stop();
        result += sink->wait();
    }

    m_detector_tracker.stop();

    return m_detector_tracker.wait() + result;
}

void Core::process(Event event)
{

    m_time_base_supervisor->process_event(event);

    if (!m_detector_tracker.accept(event)) {
        return;
    }

    m_supervisor.increase_event_count(true);

    std::queue<std::size_t> matches {};
    for (std::size_t i { 0 }; i < m_constructors.size(); i++)  {
        auto& constructor { m_constructors[i] };
        auto result { constructor->event_matches(event) };
        if ( EventConstructor::Type::NoMatch != result) {
            matches.push(i);
        }
    }

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
        m_constructors.erase(m_constructors.begin() + static_cast<ssize_t>(matches.front()));
        matches.pop();
    }
    // --- Event matches more than one constructor
    m_constructors.push_back(std::move(constructor));
    // --- Event matches either no, or more than one constructor
}

void Core::push_event(Event event)
{
    m_supervisor.increase_event_count(false);

    for (auto& sink: m_event_sinks) {
        sink->push_item(event);
    }
}

}
