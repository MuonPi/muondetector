#include "detectortracker.h"

#include "event.h"
#include "logmessage.h"
#include "abstractsource.h"
#include "detector.h"
#include "log.h"

namespace MuonPi {

AbstractDetectorTracker::AbstractDetectorTracker()
    : ThreadRunner{"DetectorTracker"}
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

DetectorTracker::DetectorTracker(std::unique_ptr<AbstractSource<LogMessage> > log_source)
    : m_log_source { std::move(log_source) }
{
}


DetectorTracker::~DetectorTracker()
{
    AbstractDetectorTracker::~AbstractDetectorTracker();
}

auto DetectorTracker::accept(const Event& event) const -> bool
{
    auto detector { m_detectors.find(event.hash()) };
    return ((detector != m_detectors.end()) && ((*detector).second->is(Detector::Status::Reliable)));
}

void DetectorTracker::process(const LogMessage& log)
{
    auto detector { m_detectors.find(log.hash()) };
    if (detector == m_detectors.end()) {
        syslog(Log::Debug, "Found new detector %X", static_cast<unsigned int>(log.hash()));
        m_detectors[log.hash()] = std::make_unique<Detector>(log);
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
    float largest { 0.0 };
    for (auto& [hash, detector]: m_detectors) {

        if (!detector->step()) {
            syslog(Log::Debug, "Deleting detector %X", static_cast<unsigned int>(hash));
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
    std::size_t i { 0 };
    while (m_log_source->has_items() && (i < 10)) {
        process(*m_log_source->next_item());
        i++;
    }
    // --- handle incoming log messages, maximum 10 at a time to prevent blocking

    std::this_thread::sleep_for( std::chrono::milliseconds{1} );
    return 0;
}
}
