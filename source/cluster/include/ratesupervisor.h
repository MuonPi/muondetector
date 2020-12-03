#ifndef RATESUPERVISOR_H
#define RATESUPERVISOR_H


#include <chrono>

namespace MuonPi {

class Event;


/**
 * @brief The RateSupervisor class
 */
class RateSupervisor
{
public:
    struct Rate {
        float m {};
        float s {};
        float n {};
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
    Rate m_allowed {};
    Rate m_current {};
    float m_history[100] {};
    bool m_dirty { false };
    std::chrono::steady_clock::time_point m_last { std::chrono::steady_clock::now() };
};

}

#endif // RATESUPERVISOR_H
