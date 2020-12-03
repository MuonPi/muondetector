#ifndef LOGMESSAGE_H
#define LOGMESSAGE_H

#include <chrono>

namespace MuonPi {

struct Location {
    double lat { 0.0 };
    double lon { 0.0 };
    double h { 0.0 };
    double prec { std::numeric_limits<double>::max() };
    double iop { std::numeric_limits<double>::min() };

    static constexpr double minimum_prec { 1.0 };
    static constexpr double maximum_iop { 1.0 };
};

/**
 * @brief The LogMessage class
 */
class LogMessage
{
public:

    LogMessage(std::size_t hash, Location location);

    /**
     * @brief hash
     * @return The hash of the detector for this event
     */
    [[nodiscard]] auto hash() const noexcept -> std::size_t;

    /**
     * @brief location The location of the detector from this log message
     * @return The location data
     */
    [[nodiscard]] auto location() const -> Location;

    /**
     * @brief time The time this log message arrived
     * @return The arrival time
     */
    [[nodiscard]] auto time() const -> std::chrono::steady_clock::time_point;

private:
    std::size_t m_hash { 0 };
    Location m_location {};
    std::chrono::steady_clock::time_point m_time { std::chrono::steady_clock::now() };
};
}

#endif // LOGMESSAGE_H
