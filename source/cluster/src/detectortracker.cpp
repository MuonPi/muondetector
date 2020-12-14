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
        Log::debug()<<"Found new detector " + std::to_string(log.hash());
        m_detectors[log.hash()] = std::make_unique<Detector>(log);
        return;
    }
    Log::debug()<<"Processing Log from " + std::to_string(log.hash());
    (*detector).second->process(log);
}

auto DetectorTracker::factor() const -> float
{
    return m_factor;
}

auto DetectorTracker::step() -> int
{
    static std::chrono::system_clock::time_point last { std::chrono::system_clock::now() };
    static std::chrono::system_clock::time_point runtime { std::chrono::system_clock::now() };
    float largest { 0.0 };
    for (auto& [hash, detector]: m_detectors) {

        if (!detector->step()) {
            Log::debug()<<"Deleting detector " + std::to_string(hash);
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
        process(*m_log_source->next_item());
    }
    // --- handle incoming log messages, maximum 10 at a time to prevent blocking

    if ((std::chrono::system_clock::now() - last) >= std::chrono::seconds{5}) {
        Log::debug()<<std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - runtime).count()) + "s: known detectors: " + std::to_string(m_detectors.size());
        last = std::chrono::system_clock::now();
    }
    std::this_thread::sleep_for( std::chrono::milliseconds{1} );
    return 0;
}
}
