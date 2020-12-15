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
    static system_clock::time_point last { system_clock::now() };
    static system_clock::time_point start { system_clock::now() };

    if ((system_clock::now() - last) > seconds{5}) {
        Log::debug()<<"runtime: " + std::to_string(duration_cast<seconds>(system_clock::now() - start).count()) + "s timeout: " + std::to_string(duration_cast<milliseconds>(m_timeout).count()) + "ms";
        last = system_clock::now();
    }
    return 0;
}

}
