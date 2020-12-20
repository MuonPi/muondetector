#ifndef DETECTORLOG_H
#define DETECTORLOG_H

#include <chrono>

namespace MuonPi {


/**
 * @brief The DetectorLog class
 * Holds information about accumulated statistics and gathered info about a detector
 */
class DetectorLog
{
public:

    /**
     * @brief DetectorLog
     * @param hash The hash of the detector identifier
     */
    DetectorLog(std::size_t hash);

    DetectorLog() noexcept;

    DetectorLog(const DetectorLog& other);
    DetectorLog(DetectorLog&& other);

    /**
     * @brief hash
     * @return The hash of the detector for this event
     */
    [[nodiscard]] auto hash() const noexcept -> std::size_t;

    /**
     * @brief deadtime
     * @return The dead time of the detector in the interval
     */
    [[nodiscard]] auto deadtime() const noexcept -> double { return m_deadtime; }

    /**
     * @brief active
     * @return the detector was active in the interval
     */
    [[nodiscard]] auto active() const noexcept -> bool { return m_active; }

    /**
     * @brief mean_eventrate
     * @return The mean event rate of the detector in the interval
     */
    [[nodiscard]] auto mean_eventrate() const noexcept -> double { return m_mean_eventrate; }

    /**
     * @brief mean_pulselength
     * @return The mean pulse length of the detector in the interval
     */
    [[nodiscard]] auto mean_pulselength() const noexcept -> double { return m_mean_pulselength; }

	/**
     * @brief time The time this log message was created
     * @return The creation time
     */
    [[nodiscard]] auto time() const -> std::chrono::system_clock::time_point;

	/**
     * @brief valid Indicates whether this message is valid
     * @return message is valid
     */
    [[nodiscard]] auto valid() const -> bool;

private:
    std::size_t m_hash { 0 };
    double m_deadtime { 0.0 };
	bool m_active { false };
	double m_mean_eventrate { 0.0 };
	double m_mean_pulselength { 0.0 };
	
	std::chrono::system_clock::time_point m_time { std::chrono::system_clock::now() };

    bool m_valid { true };
};
}

#endif // DETECTORLOG_H
