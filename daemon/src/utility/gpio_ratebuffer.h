#ifndef GPIO_RATEBUFFER_H
#define GPIO_RATEBUFFER_H

#include "core/event_bus.h"
#include "data/events/gpio_event.h"
#include "data/events/interval_event.h"

#include <chrono>
#include <limits>
#include <list>
#include <map>
#include <muondetector_structs.h>
#include <queue>

using namespace std::literals;

constexpr double MAX_AVG_RATE{500.};
constexpr unsigned long MAX_BURST_MULTIPLICITY{20};
constexpr std::chrono::microseconds MAX_BUFFER_TIME{30s};
constexpr std::chrono::microseconds MAX_DEADTIME{static_cast<unsigned long>(1e+6 / MAX_AVG_RATE)};
constexpr std::chrono::microseconds DEADTIME_INCREMENT{50};
constexpr std::chrono::microseconds COINCIDENCE_WINDOW{50};

constexpr auto invalid_time = TimestampClockType::time_point::min();

struct RateSample {
    EventTime timestamp;
    double rate_hz;
};

class EdgeFilter {
  public:
    EdgeFilter(const std::optional<EventEdge>& filter_edge);
    ~EdgeFilter() = default;
    auto accept(const GpioEvent& event) -> bool;

  private:
    std::optional<EventEdge> m_filter_edge;
};

class DeadtimeFilter {
  public:
    DeadtimeFilter(std::chrono::microseconds deadtime);
    ~DeadtimeFilter() = default;
    auto accept(EventTime ts) -> bool;

    void reset();

  private:
    EventTime m_lastAccepted = invalid_time;
    std::chrono::microseconds m_deadtime;
};

class SlidingRateEstimator {
  public:
    SlidingRateEstimator(const std::chrono::seconds window);

    void add(EventTime ts);

    void cleanup(EventTime now);

    double currentRateHz(EventTime now) const;

    void clear();

  private:
    std::deque<EventTime> m_snapshot;
    std::deque<EventTime> m_events;

    std::chrono::seconds m_window;
};

class RateHistory {
  public:
    void addSample(EventTime ts, double rate);

    auto samples() const -> const std::deque<RateSample>&;

    auto startTime() const -> EventTime;

    void clear();

    // auto begin() const { return m_samples.begin(); }
    // auto end()   const { return m_samples.end(); }

  private:
    std::deque<RateSample> m_samples;
    std::chrono::hours m_historyLength{2};
};

class EventRateBuffer {
  public:
    EventRateBuffer(const std::optional<EventEdge>& filterEdge = std::nullopt,
                    const std::chrono::seconds& slidingWindow = 5s,
                    const std::chrono::microseconds& deadtime = 0us);
    ~EventRateBuffer() = default;

    [[nodiscard]]
    auto handle(const GpioEvent&) -> std::pair<bool, std::optional<IntervalEvent>>;

    [[nodiscard]]
    auto rateSamples() const -> const std::deque<RateSample>&;

    auto sampleAndRetrieve(const EventTime& now) -> std::vector<std::pair<float, float>>;

    void clear();

  private:
    auto timeSinceStart(const EventTime& tp) const -> double;
    std::optional<GPIO_SIGNAL> m_coincSig{std::nullopt};
    EventTime m_lastCoincidenceEvent{invalid_time};
    bool m_is_veto{false};

    EdgeFilter m_edgeFilter;
    SlidingRateEstimator m_rateEstimator;
    DeadtimeFilter m_deadtime;
    RateHistory m_rateHistory;

    // // deadtime state
    EventTime m_lastAcceptedEvent{invalid_time};

    std::chrono::nanoseconds m_lastInterval{0};

    // bookkeeping
    EventTime m_lastRateSample{invalid_time};

    // statistics
    // uint64_t m_totalEvents{0};
    // uint64_t m_acceptedEvents{0};
    uint64_t m_rejectedDeadtime{0};
    uint64_t m_rejectedCoincidence{0};
    EventTime m_startTime;
    EventTime m_latestSampleTime;
};

#endif // GPIO_RATEBUFFER_H
