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


/**
 * @brief The StateSupervisor class Supervises the program and collects metadata
 */
class StateSupervisor
{
public:
    /**
     * @brief StateSupervisor
     * @param log_sinks The specific log sinks to send metadata to
     */
    StateSupervisor(std::vector<std::shared_ptr<AbstractSink<ClusterLog>>> log_sinks);

    /**
     * @brief time_status Update the current timeout used
     * @param timeout the timeout in ms
     */
    void time_status(std::chrono::milliseconds timeout);

    /**
     * @brief detector_status Update the status of one detector
     * @param hash The hashed detector identifier
     * @param status The new status of the detector
     */
    void detector_status(std::size_t hash, Detector::Status status);

    /**
     * @brief step Gets called from the core class.
     * @return 0 if everything is okay
     */
    [[nodiscard]] auto step() -> int;

    /**
     * @brief increase_event_count gets called when an event arrives or gets processed
     * @param incoming true if the event is incoming, false if it a processed one
     * @param n The coincidence level of the event. Only used for processed events.
     */
    void increase_event_count(bool incoming, std::size_t n = 1);

    /**
     * @brief set_queue_size Update the current event constructor buffer size.
     * @param size The current size
     */
    void set_queue_size(std::size_t size);

    /**
     * @brief add_thread Add a thread to supervise. If this thread quits or has an error state, the main event loop will stop.
     * @param thread Pointer to the thread to supervise
     */
    void add_thread(ThreadRunner* thread);


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
    std::chrono::steady_clock::time_point m_last { std::chrono::steady_clock::now() };

};

}

#endif // STATESUPERVISOR_H
