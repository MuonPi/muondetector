#ifndef STATESUPERVISOR_H
#define STATESUPERVISOR_H

#include "threadrunner.h"
#include "detector.h"
#include "utility.h"
#include "clusterlog.h"
#include "abstractsink.h"

#include <cinttypes>
#include <vector>
#include <map>
#include <chrono>
#include <fstream>

namespace MuonPi {


class StateSupervisor
{
public:
    StateSupervisor(std::vector<std::shared_ptr<AbstractSink<ClusterLog>>> log_sinks);

    void time_status(std::chrono::milliseconds timeout);
    void detector_status(std::size_t hash, Detector::Status status);

    [[nodiscard]] auto step() -> int;

    void increase_event_count(bool incoming, std::size_t n = 1);
    void set_queue_size(std::size_t size);

    void add_thread(ThreadRunner* thread);

    void set_detector_status(std::size_t total, std::size_t reliable);

private:
    std::map<std::size_t, Detector::Status> m_detectors;
    std::chrono::milliseconds m_timeout;
    std::chrono::system_clock::time_point m_start { std::chrono::system_clock::now() };

    std::size_t m_incoming_count { 0 };
    std::size_t m_outgoing_count { 0 };

    RateMeasurement<100, 5000> m_incoming_rate {};
    RateMeasurement<100, 5000> m_outgoing_rate {};

    std::vector<ThreadRunner*> m_threads;

    std::vector<std::shared_ptr<AbstractSink<ClusterLog>>> m_log_sinks;

    ClusterLog::Data m_current_data;

};

}

#endif // STATESUPERVISOR_H
