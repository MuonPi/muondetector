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

private:
    Rate m_allowed {};
    Rate m_current {};
    float m_history[100] {};
    std::chrono::steady_clock::time_point m_last { std::chrono::steady_clock::now() };
};

}

#endif // RATESUPERVISOR_H
