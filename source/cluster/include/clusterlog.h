#ifndef CLUSTERLOG_H
#define CLUSTERLOG_H

#include <chrono>
#include <string>
#include <map>

namespace MuonPi {


/**
 * @brief The ClusterLog class
 */
class ClusterLog
{
public:
    struct Data {
        std::int_fast64_t timeout { 0 }; //!< The current timeout for event constructors, in ms
        struct {
            double single_in { 0 }; //!< The mean rate of incoming events
            double l1_out { 0 }; //!< The mean rate of outgoing l1 events
        } frequency;

        std::size_t incoming { 0 }; //!< The number of incoming messages in the last interval
        std::map<std::size_t, std::size_t> outgoing {}; //!< The number of outgoing messages in the last interval, separated by coincidence level
        std::size_t buffer_length { 0 }; //!< the current number of event constructors in the buffer
        std::size_t total_detectors { 0 }; //!< The current total number of tracked detectors
        std::size_t reliable_detectors { 0 }; //!< The current number of tracked detectors deemed reliable
        std::size_t maximum_n { 0 }; //!< The maximum coincidence level found so far since program start
    };

    /**
     * @brief ClusterLog
     * @param data The log data
     */
    ClusterLog(Data data);

    /**
     * @brief ClusterLog constructs an invalid object
     */
    ClusterLog();

    /**
     * @brief data Accesses the data from the object
     * @return
     */
    [[nodiscard]] auto data() -> Data;

    /**
     * @brief time The time this log object was created
     * @return The creation time
     */
    [[nodiscard]] auto time() const -> std::chrono::system_clock::time_point;

private:
    Data m_data;
	std::chrono::system_clock::time_point m_time { std::chrono::system_clock::now() };
	bool m_valid { true };
};

	
} // namespace MuonPi

#endif // CLUSTERLOG_H
