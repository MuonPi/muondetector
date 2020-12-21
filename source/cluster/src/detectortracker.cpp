#include "detectortracker.h"

#include "event.h"
#include "detectorinfo.h"
#include "detectorlog.h"
#include "abstractsource.h"
#include "detector.h"
#include "log.h"

#include "statesupervisor.h"

namespace MuonPi {


DetectorTracker::DetectorTracker(std::vector<std::shared_ptr<AbstractSource<DetectorInfo>>> log_sources, std::vector<std::shared_ptr<AbstractSink<DetectorLog>>> log_sinks, StateSupervisor &supervisor)
    : ThreadRunner{"DetectorTracker"}
    , m_supervisor { supervisor }
    , m_log_sources { std::move(log_sources) }
    , m_log_sinks { std::move(log_sinks) }
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

void DetectorTracker::process(const DetectorInfo& log)
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
    using namespace std::chrono;
	
	double largest { 1.0 };
    std::size_t reliable { 0 };
    for (auto& [hash, detector]: m_detectors) {

        if (!detector->step()) {
            m_supervisor.detector_status(hash, Detector::Status::Deleted);
            m_delete_detectors.push(hash);
            continue;
        }
        if (detector->is(Detector::Status::Reliable)) {
            reliable++;
            if (detector->factor() > largest) {
                largest = detector->factor();
            }
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

    
    // +++ push detector log messages at regular interval
    static steady_clock::time_point last { steady_clock::now() };
    steady_clock::time_point now { steady_clock::now() };

    constexpr std::chrono::seconds detector_log_interval{300};

    if ((now - last) >= detector_log_interval) {
        last = now;

		for (auto& [hash, detector]: m_detectors) {
			DetectorLog log(detector->current_log_data());
			for (auto& sink: m_log_sinks) {
				sink->push_item(log);
			}
		}
    }
    // --- push detector log messages at regular interval
    
    // TODO: implement dead time in the detector log and an immediate logging if a detector becomes active or inactive
    
    std::this_thread::sleep_for( std::chrono::milliseconds{1} );
    return 0;
}
}
