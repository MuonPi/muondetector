#ifndef DETECTOR_H
#define DETECTOR_H

#include "threadrunner.h"
#include "logmessage.h"

#include <memory>
#include <chrono>
#include <future>

namespace MuonPi {


// +++ forward declarations
class RateSupervisor;
class Event;
// --- forward declarations




/**
 * @brief The Detector class
 * Represents a connected detector.
 * Scans the message rate and deletes the runtime objects from memory if the detector has not been active for some time.
 */
class Detector : public ThreadRunner
{
public:
    enum class Status {
        Unreliable,
        Reliable,
        Quitting
    };

    class Listener
    {
    public:
        virtual ~Listener();
        virtual void factor_changed(std::size_t hash, float factor) = 0;
        virtual void detector_status_changed(std::size_t hash, Status status) = 0;
    };
    /**
     * @brief Detector
     * @param listener The event listener for changes to this detector
     * @param initial_log The initial log message from which this detector object originates
     */
    Detector(Listener* listener, const std::unique_ptr<LogMessage>& initial_log);

    ~Detector() override;

    /**
     * @brief process Processes an event message. This means it calculates the event rate from this detector.
     * @param event the event to process
     * @return true if the event is part of this detector
     */
    [[nodiscard]] auto process(const std::unique_ptr<Event>& event) -> bool;

    /**
     * @brief process Processes a log message. Checks for regular log messages and warns the event listener if they are delayed or have subpar location accuracy.
     * @param log The logmessage to process
     * @return true if the log message matches this detector
     */
    [[nodiscard]] auto process(const std::unique_ptr<LogMessage>& log) -> bool;

protected:
    /**
     * @brief step Reimplemented from ThreadRunner
     * @return true
     */
    [[nodiscard]] auto step() -> bool override;

    /**
     * @brief set_status Sets the status of this detector and sends the status to the listener if it has changed.
     * @param status The status to set
     */
    void set_status(Status status);

    /**
     * @brief is Checks the current detector status against a value
     * @param status The status to compare against
     * @return true if the detector has the status asked for in the parameter
     */
    [[nodiscard]] auto is(Status status) const -> bool;

private:

    std::atomic<Status> m_status { Status::Unreliable };
    std::atomic<bool> m_tick { false };

    Location m_location {};
    std::size_t m_hash { 0 };

    Listener* m_listener { nullptr };
    std::unique_ptr<RateSupervisor> m_supervisor { nullptr };
    std::atomic<std::chrono::steady_clock::time_point> m_last_log { std::chrono::steady_clock::now() };

    static constexpr std::chrono::steady_clock::duration s_log_interval { std::chrono::minutes { 5 } };
    static constexpr std::chrono::steady_clock::duration s_wait_interval { s_log_interval * 3 };
};

}

#endif // DETECTOR_H
