#ifndef DETECTOR_H
#define DETECTOR_H

#include "threadrunner.h"
#include "detectorinfo.h"
#include "detectorlog.h"
#include "utility.h"
#include "userinfo.h"

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
private:
    static constexpr std::size_t s_history_length { 10 };
	static constexpr std::size_t s_time_interval { 2000 };
public:

	using CurrentRateType=RateMeasurement<s_history_length, s_time_interval>;
	using MeanRateType=RateMeasurement<s_history_length * 100, s_time_interval>;
	
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
    Detector(const DetectorInfo& initial_log, StateSupervisor& supervisor);


    /**
     * @brief process Processes an event message. This means it calculates the event rate from this detector.
     * @param event the event to process
     */
    void process(const Event& event);

    /**
     * @brief process Processes a detector info message. Checks for regular log messages and warns the event listener if they are delayed or have subpar location accuracy.
     * @param info The detector info to process
     */
    void process(const DetectorInfo& info);

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
	
//	[[nodiscard]] auto mean_rate() const -> MeanRateType;
//	[[nodiscard]] auto current_rate() const -> CurrentRateType;
	
	[[nodiscard]] auto current_log_data() -> DetectorLog;
	
    /**
     * @brief data Accesses the user info from the object
     * @return the UserInfo struct
     */
    [[nodiscard]] auto user_info() const -> UserInfo;

	
protected:
    /**
     * @brief set_status Sets the status of this detector and sends the status to the listener if it has changed.
     * @param status The status to set
     */
    void set_status(Status status);

private:

    std::atomic<Status> m_status { Status::Unreliable };

    Location m_location {};
    std::size_t m_hash { 0 };
	UserInfo m_userinfo { };

    std::chrono::system_clock::time_point m_last_log { std::chrono::system_clock::now() };

    static constexpr std::chrono::system_clock::duration s_log_interval { std::chrono::seconds { 90 } };
    static constexpr std::chrono::system_clock::duration s_quit_interval { s_log_interval * 3 };

    StateSupervisor& m_state_supervisor;

    CurrentRateType m_current_rate {};
    MeanRateType m_mean_rate {};
	
	DetectorLog::Data m_current_data;
	std::uint16_t m_last_ublox_counter {};
	long int m_current_ublox_progress {};
	
	Ringbuffer<double, 100> m_pulselength {};

    double m_factor { 1.0 };
};

}

#endif // DETECTOR_H
