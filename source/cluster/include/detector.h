#ifndef DETECTOR_H
#define DETECTOR_H

#include "threadrunner.h"
#include "ratesupervisor.h"
#include "detectorlog.h"

#include <memory>
#include <chrono>
#include <future>

namespace MuonPi {


// +++ forward declarations
class Event;
class StateSupervisor;
// --- forward declarations




/**
 * @brief The Detector class
 * Represents a connected detector.
 * Scans the message rate and deletes the runtime objects from memory if the detector has not been active for some time.
 */
class Detector
{
public:
    enum class Status {
        Created,
        Deleted,
        Unreliable,
        Reliable
    };
    /**
     * @brief Detector
     * @param initial_log The initial log message from which this detector object originates
     */
    Detector(const DetectorLog& initial_log, StateSupervisor& supervisor);


    /**
     * @brief process Processes an event message. This means it calculates the event rate from this detector.
     * @param event the event to process
     */
    void process(const Event& event);

    /**
     * @brief process Processes a log message. Checks for regular log messages and warns the event listener if they are delayed or have subpar location accuracy.
     * @param log The logmessage to process
     */
    void process(const DetectorLog& log);

    /**
     * @brief is Checks the current detector status against a value
     * @param status The status to compare against
     * @return true if the detector has the status asked for in the parameter
     */
    [[nodiscard]] auto is(Status status) const -> bool;

    /**
     * @brief factor The current factor from the event supervisor
     * @return the numeric factor
     */
    [[nodiscard]] auto factor() const -> double;
    /**
     * @brief step Reimplemented from ThreadRunner
     * @return true
     */
    [[nodiscard]] auto step() -> bool;

protected:
    /**
     * @brief set_status Sets the status of this detector and sends the status to the listener if it has changed.
     * @param status The status to set
     */
    void set_status(Status status);

private:

    std::atomic<Status> m_status { Status::Unreliable };
    std::atomic<bool> m_tick { false };

    Location m_location {};
    std::size_t m_hash { 0 };

    std::unique_ptr<RateSupervisor> m_supervisor { nullptr };
    std::chrono::system_clock::time_point m_last_log { std::chrono::system_clock::now() };

    static constexpr std::chrono::system_clock::duration s_log_interval { std::chrono::seconds { 90 } };
    static constexpr std::chrono::system_clock::duration s_quit_interval { s_log_interval * 3 };

    StateSupervisor& m_state_supervisor;
};

}

#endif // DETECTOR_H
