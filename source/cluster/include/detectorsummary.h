#ifndef DETECTORSUMMARY_H
#define DETECTORSUMMARY_H

#include "userinfo.h"

#include <chrono>

namespace MuonPi {

/**
 * @brief The DetectorSummary class
 * Holds information about accumulated statistics and gathered info about a detector
 */
class DetectorSummary
{
public:
    struct Data {
        double deadtime { 0.0 };
        bool active { false };
        double mean_eventrate { 0.0 };
        double mean_pulselength { 0.0 };
        std::int64_t ublox_counter_progress { 0 };
        std::uint64_t incoming { 0UL };
    };

    /**
     * @brief DetectorLog
     * @param hash The hash of the detector identifier
     * @param data The data struct to be provided
     */
    DetectorSummary(std::size_t hash, UserInfo user_info, Data data);

    DetectorSummary() noexcept;

    DetectorSummary(const DetectorSummary& other);
    DetectorSummary(DetectorSummary&& other);

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
     * @return data struct
     */
    [[nodiscard]] auto data() -> Data;

    /**
     * @brief data Accesses the user info from the object
     * @return the UserInfo struct
     */
    [[nodiscard]] auto user_info() const -> UserInfo;

private:
    std::size_t m_hash { 0 };

    std::chrono::system_clock::time_point m_time { std::chrono::system_clock::now() };
    Data m_data { };
    UserInfo m_userinfo { };
    bool m_valid { true };
};
}

#endif // DETECTORSUMMARY_H
