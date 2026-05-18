#include "utility/gpio_ratebuffer.h"

#include "core/event_bus.h"
#include "core/logging/logger.h"
#include "data/events/gpio_event.h"
#include "data/events/interval_event.h"
#include "gpio_pin_definitions.h"

#include <algorithm>
#include <iostream>
#include <vector>

EdgeFilter::EdgeFilter(const std::optional<EventEdge>& filter_edge) : m_filter_edge{filter_edge} {
}

auto EdgeFilter::accept(const GpioEvent& event) -> bool {
    if (m_filter_edge.has_value()) {
        return event.edge == m_filter_edge.value();
    }
    return true;
}

DeadtimeFilter::DeadtimeFilter(std::chrono::microseconds deadtime) : m_deadtime{deadtime} {
}

bool DeadtimeFilter::accept(EventTime ts) {
    if (m_lastAccepted == invalid_time) {
        m_lastAccepted = ts;
        return true;
    }

    auto dt = ts - m_lastAccepted;

    if (dt < m_deadtime)
        return false;

    m_lastAccepted = ts;
    return true;
}

void DeadtimeFilter::reset() {
    m_lastAccepted = invalid_time;
}

SlidingRateEstimator::SlidingRateEstimator(const std::chrono::seconds window) : m_window{window} {
}

void SlidingRateEstimator::add(EventTime ts) {
    m_events.push_back(ts);

    while (!m_events.empty() && ts - m_events.front() > m_window) {
        m_events.pop_front();
    }
}

void SlidingRateEstimator::cleanup(EventTime now) {
    while (!m_events.empty() && now - m_events.front() > m_window) {
        m_events.pop_front();
    }
}

double SlidingRateEstimator::currentRateHz(EventTime now) const {
    if (m_events.size() < 2)
        return 0.0;

    // find first valid element without modifying deque
    auto it = m_events.begin();
    while (it != m_events.end() && now - *it > m_window) {
        ++it;
    }

    auto valid_count = std::distance(it, m_events.end());
    if (valid_count < 2)
        return 0.0;

    auto dt = m_events.back() - *it;
    double sec = std::chrono::duration<double>(dt).count();

    if (sec <= 0.0)
        return 0.0;

    return static_cast<double>(valid_count) / sec;
}

void SlidingRateEstimator::clear() {
    m_events.clear();
}

void RateHistory::addSample(EventTime ts, double rate) {
    m_samples.push_back({ts, rate});

    while (!m_samples.empty() && ts - m_samples.front().timestamp > m_historyLength) {
        m_samples.pop_front();
    }
}

void RateHistory::clear() {
    m_samples.clear();
}

auto RateHistory::samples() const -> const std::deque<RateSample>& {
    return m_samples;
}

EventRateBuffer::EventRateBuffer(const std::optional<EventEdge>& filterEdge,
                                 const std::chrono::seconds& slidingWindow,
                                 const std::chrono::microseconds& deadtime)
    : m_edgeFilter{filterEdge}
    , m_rateEstimator{slidingWindow}
    , m_deadtime{deadtime}
    , m_startTime{EventTime::clock::now()} {
}

auto EventRateBuffer::handle(const GpioEvent& event)
    -> std::pair<bool, std::optional<IntervalEvent>> {
    // 1. edge filter
    if (m_edgeFilter.accept(event) == false) {
        return {false, std::nullopt};
    }

    // If coincidence logic is enabled
    if (m_coincSig.has_value()) {
        // 2. coincidence bookkeeping (state only)
        bool coincident = false;
        if (event.gpio_signal == m_coincSig.value()) {
            m_lastCoincidenceEvent = event.timestamp;
            return {false, std::nullopt};
        }
        if (m_lastCoincidenceEvent != invalid_time) {
            auto dt_us = std::chrono::duration_cast<std::chrono::microseconds>(
                             event.timestamp - m_lastCoincidenceEvent)
                             .count();

            coincident = std::abs(dt_us) <= COINCIDENCE_WINDOW.count();
        }
        if (m_is_veto && coincident) {
            ++m_rejectedCoincidence;
            return {false, std::nullopt};
        }
        if (!m_is_veto && !coincident) {
            ++m_rejectedCoincidence;
            return {false, std::nullopt};
        }
    }

    // 3. deadtime filter (DELEGATED)
    if (!m_deadtime.accept(event.timestamp)) {
        ++m_rejectedDeadtime;
        return {false, std::nullopt};
    }

    // 4. accepted event bookkeeping
    m_lastAcceptedEvent = event.timestamp;

    m_rateEstimator.add(event.timestamp);

    // 7. interval output
    if (m_lastInterval != 0ns) {
        return {true, IntervalEvent{.sig = event.gpio_signal, .interval = m_lastInterval}};
    }

    return {true, std::nullopt};
}

auto EventRateBuffer::sampleAndRetrieve(const EventTime& now)
    -> std::vector<std::pair<float, float>> {
    // 6. long-term history (decoupled)
    double rate = m_rateEstimator.currentRateHz(now);
    m_rateHistory.addSample(now, rate);

    auto it = m_rateHistory.samples().rbegin();
    auto end = m_rateHistory.samples().rend();
    std::vector<std::pair<float, float>> out;
    for (; it != end; ++it) {
        if (it->timestamp < m_latestSampleTime)
            break;

        out.emplace_back(
            static_cast<float>(std::chrono::duration<double>(it->timestamp - m_startTime).count()),
            static_cast<float>(it->rate_hz));
    }

    m_latestSampleTime = now;
    std::reverse(out.begin(), out.end());
    return out;
}

auto EventRateBuffer::timeSinceStart(const EventTime& tp) const -> double {
    return (tp - m_startTime).count() * 1e-9;
}

void EventRateBuffer::clear() {
    m_rateEstimator.clear();
    m_rateHistory.clear();

    m_deadtime.reset();

    m_lastAcceptedEvent = invalid_time;
    m_lastCoincidenceEvent = invalid_time;
    m_lastRateSample = invalid_time;
    m_startTime = EventTime::clock::now();
    m_latestSampleTime = m_startTime;
}

auto EventRateBuffer::rateSamples() const -> const std::deque<RateSample>& {
    return m_rateHistory.samples();
}
