#ifndef RATESUPERVISOR_H
#define RATESUPERVISOR_H


#include <chrono>
#include <atomic>

namespace MuonPi {

class Event;


/**
 * @brief The RateSupervisor class
 */
class RateSupervisor
{
public:
    struct Rate {
        float m {}; //!< The mean value
        float s {}; //!< The standard deviation
        float n {}; //!< The factor for the standard deviation
    };

    /**
     * @brief RateSupervisor
     * @param allowable The allowable rate definition
     */
    RateSupervisor(Rate allowable);

    /**
     * @brief tick
     * @param message whether this tick contains a message
     */
    void tick(bool message);

    /**
     * @brief current
     * @return The current Rate measurement
     */
    [[nodiscard]] auto current() const -> Rate;

    /**
     * @brief factor The factor of the current rate is outside the specified tolerance
     * @return The factor for the standard deviation of the allowable rate
     */
    [[nodiscard]] auto factor() const -> float;

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

    static constexpr std::size_t s_history_length { 100 };

    Rate m_allowed {};
    Rate m_current {};
    float m_history[s_history_length] {}; //!< contains the time differences between each points, in microseconds
    std::atomic<bool> m_dirty { false };
    std::chrono::steady_clock::time_point m_last { std::chrono::steady_clock::now() };
};

}

#endif // RATESUPERVISOR_H
