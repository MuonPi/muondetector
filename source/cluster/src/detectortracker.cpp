#include "detectortracker.h"

#include "event.h"
#include "logmessage.h"
#include "abstractsource.h"
#include "detector.h"
#include "log.h"

#include "statesupervisor.h"

namespace MuonPi {

AbstractDetectorTracker::AbstractDetectorTracker(StateSupervisor& supervisor)
    : ThreadRunner{"DetectorTracker"}
    , m_supervisor { supervisor }
{
    start();
}

AbstractDetectorTracker::~AbstractDetectorTracker() = default;

auto AbstractDetectorTracker::accept(const Event& /*event*/) const -> bool
{
    return true;
}

auto AbstractDetectorTracker::factor() const -> float
{
    return 1.0;
}


DetectorTracker::DetectorTracker(std::unique_ptr<AbstractSource<LogMessage> > log_source, StateSupervisor &supervisor)
    : AbstractDetectorTracker { supervisor }
    , m_log_source { std::move(log_source) }
{
}


DetectorTracker::~DetectorTracker()
{
    AbstractDetectorTracker::~AbstractDetectorTracker();
}

auto DetectorTracker::accept(const Event& event) const -> bool
{
    auto detector { m_detectors.find(event.hash()) };
    if (detector != m_detectors.end()) {
        (*detector).second->process(event);
        return ((*detector).second->is(Detector::Status::Reliable));
    }
    return false;
}

void DetectorTracker::process(const LogMessage& log)
{
    auto detector { m_detectors.find(log.hash()) };
    if (detector == m_detectors.end()) {
        m_supervisor.detector_status(log.hash(), Detector::Status::Created);
       m_detectors[log.hash()] = std::make_unique<Detector>(log, m_supervisor);
        return;
    }
    (*detector).second->process(log);
}

auto DetectorTracker::factor() const -> float
{
    return m_factor;
}

auto DetectorTracker::step() -> int
{
    if (m_log_source->state() <= ThreadRunner::State::Stopped) {
        Log::error()<<"The Log source stopped.";
        return -1;
    }
    float largest { 0.0 };
    for (auto& [hash, detector]: m_detectors) {

        if (!detector->step()) {
            m_supervisor.detector_status(hash, Detector::Status::Deleted);
            m_delete_detectors.push(hash);
            continue;
        }
        if (detector->factor() > largest) {
            largest = detector->factor();
        }
    }
    m_factor = largest;
    while (!m_delete_detectors.empty()) {
        m_detectors.erase(m_delete_detectors.front());
        m_delete_detectors.pop();
    }

    // +++ handle incoming log messages, maximum 10 at a time to prevent blocking
    if (m_log_source->has_items()) {
        process(m_log_source->next_item());
    }
    // --- handle incoming log messages, maximum 10 at a time to prevent blocking

    std::this_thread::sleep_for( std::chrono::milliseconds{1} );
    return 0;
}
}
