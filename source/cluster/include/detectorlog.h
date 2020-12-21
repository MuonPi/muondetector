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
    struct Data {
	    double deadtime { 0.0 };
		bool active { false };
		double mean_eventrate { 0.0 };
		double mean_pulselength { 0.0 };
		unsigned long incoming { 0UL };
	};
	
	/**
     * @brief DetectorLog
     * @param hash The hash of the detector identifier
	 * @param data The data struct to be provided
     */
	DetectorLog(std::size_t hash, Data data);

    DetectorLog() noexcept;

    DetectorLog(const DetectorLog& other);
    DetectorLog(DetectorLog&& other);

    /**
     * @brief hash
     * @return The hash of the detector for this event
     */
    [[nodiscard]] auto hash() const noexcept -> std::size_t;

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
	
    /**
     * @brief data Accesses the data from the object
     * @return
     */
    [[nodiscard]] auto data() -> Data;
	

private:
    std::size_t m_hash { 0 };
	
	std::chrono::system_clock::time_point m_time { std::chrono::system_clock::now() };
	Data m_data { };
    bool m_valid { true };
};
}

#endif // DETECTORLOG_H
