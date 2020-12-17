#include "detectortracker.h"

#include "event.h"
#include "detectorlog.h"
#include "abstractsource.h"
#include "detector.h"
#include "log.h"

#include "statesupervisor.h"

namespace MuonPi {



DetectorTracker::DetectorTracker(std::vector<std::shared_ptr<AbstractSource<DetectorLog>>> log_sources, StateSupervisor &supervisor)
    : ThreadRunner{"DetectorTracker"}
    , m_supervisor { supervisor }
    , m_log_sources { std::move(log_sources) }
{
    start();
}



auto DetectorTracker::accept(Event& event) const -> bool
{
    auto detector { m_detectors.find(event.hash()) };
    if (detector != m_detectors.end()) {
        (*detector).second->process(event);

        event.set_detector((*detector).second);

        return ((*detector).second->is(Detector::Status::Reliable));
    }
    return false;
}

void DetectorTracker::process(const DetectorLog& log)
{
    auto detector { m_detectors.find(log.hash()) };
    if (detector == m_detectors.end()) {
        m_supervisor.detector_status(log.hash(), Detector::Status::Created);
       m_detectors[log.hash()] = std::make_unique<Detector>(log, m_supervisor);
        return;
    }
    (*detector).second->process(log);
}

auto DetectorTracker::factor() const -> double
{
    return m_factor;
}

auto DetectorTracker::get(std::size_t hash) const -> std::shared_ptr<Detector>
{
    if (m_detectors.find(hash) == m_detectors.end()) {
        return nullptr;
    }
    return m_detectors.at(hash);
}

auto DetectorTracker::step() -> int
{
    double largest { 1.0 };
    for (auto& [hash, detector]: m_detectors) {

        if (!detector->step()) {
            m_supervisor.detector_status(hash, Detector::Status::Deleted);
            m_delete_detectors.push(hash);
            continue;
        }
        if ((detector->is(Detector::Status::Reliable)) && (detector->factor() > largest)) {
            largest = detector->factor();
        }
    }
    m_factor = largest;
    while (!m_delete_detectors.empty()) {
        m_detectors.erase(m_delete_detectors.front());
        m_delete_detectors.pop();
    }

    // +++ handle incoming log messages, maximum 10 at a time to prevent blocking
    for (auto& source: m_log_sources) {
        if (source->has_items()) {
            process(source->next_item());
        }
    }
    // --- handle incoming log messages, maximum 10 at a time to prevent blocking

    std::this_thread::sleep_for( std::chrono::milliseconds{1} );
    return 0;
}
}
