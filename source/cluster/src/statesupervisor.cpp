#include "statesupervisor.h"

#include "log.h"

#include <sstream>

namespace MuonPi {


void StateSupervisor::time_status(std::chrono::milliseconds timeout)
{
    m_timeout = timeout;
}

void StateSupervisor::detector_status(std::size_t hash, Detector::Status status)
{
    std::string output {};
    std::ostringstream out { output };
    out
            << "Detector status changed: "
            << std::hex
            << hash
            << " -> "
            ;

    switch (status) {
    case Detector::Status::Created:
        out<<"Created";
        break;
    case Detector::Status::Deleted:
        out<<"Deleted";
        break;
    case Detector::Status::Reliable:
        out<<"Reliable";
        break;
    case Detector::Status::Unreliable:
        out<<"Unreliable";
        break;
    }

    Log::debug() << out.str();

    m_detectors[hash] = status;
    if (status == Detector::Status::Deleted) {
        if (m_detectors.find(hash) != m_detectors.end()) {
            m_detectors.erase(hash);
        }
    }

    std::size_t reliable { 0 };
    for (auto& [hash, detector]: m_detectors) {
        if (detector == Detector::Status::Reliable) {
            reliable++;
        }
    }
    if (status != Detector::Status::Created) {
        Log::debug()<<"known detectors: " + std::to_string(m_detectors.size()) + " (reliable: " + std::to_string(reliable) + ")";
    }
}


auto StateSupervisor::step() -> int
{
    using namespace std::chrono;

    if (m_outgoing_rate.step()) {
        m_incoming_rate.step();
        Log::debug()<<"runtime: " + std::to_string(duration_cast<seconds>(system_clock::now() - m_start).count()) + "s timeout: " + std::to_string(duration_cast<milliseconds>(m_timeout).count()) + "ms ( -> " + std::to_string(m_incoming_count) + " [ " + std::to_string(m_queue_size) + " ] " + std::to_string(m_outgoing_count) + " -> ) " + " i rate: " + std::to_string(m_incoming_rate.mean()) + "Hz outgoing rate: " + std::to_string(m_outgoing_rate.mean()) + "Hz";
        m_incoming_count = 0;
        m_outgoing_count = 0;
    }
    return 0;
}

void StateSupervisor::increase_event_count(bool incoming)
{
    if (incoming) {
        m_incoming_count++;
        m_incoming_rate.increase_counter();
    } else {
        m_outgoing_count++;
        m_outgoing_rate.increase_counter();
    }
}

void StateSupervisor::set_queue_size(std::size_t size)
{
    m_queue_size = size;
}
}
