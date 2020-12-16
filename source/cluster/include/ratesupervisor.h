#ifndef RATESUPERVISOR_H
#define RATESUPERVISOR_H


#include "event.h"
#include "utility.h"

#include <chrono>
#include <atomic>
#include <array>

namespace MuonPi {


/**
 * @brief The RateSupervisor class
 */
class RateSupervisor
{
public:
    /**
     * @brief tick
     * @param message whether this tick contains a message
     */
    void tick(bool message);

    /**
     * @brief factor The factor of the current rate is outside the specified tolerance
     * @return The factor for the standard deviation of the allowable rate
     */
    [[nodiscard]] auto factor() const -> double;

    /**
     * @brief dirty Whether the factor changed. This also resets the dirty flag.
     * @return true if the factor changed.
     */
    [[nodiscard]] auto dirty() -> bool;

private:
    /**
     * @brief mark_dirty Marks this RateSupervisor to have a changed value
     */
    void mark_dirty();

    static constexpr std::size_t s_history_length { 10 };

    RateMeasurement<s_history_length, 2000> m_current {};
    RateMeasurement<s_history_length * 100, 2000> m_mean {};

    double m_factor { 1.0 };


    std::atomic<bool> m_dirty { false };
};

}

#endif // RATESUPERVISOR_H
