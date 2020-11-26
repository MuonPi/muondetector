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
    /**
     * @brief RateSupervisor
     * @param mean
     * @param std_deviation
     */
    RateSupervisor(float mean, float std_deviation, float allowable_factor);

    /**
     * @brief tick
     * @param message
     */
    void tick(bool message);

    /**
     * @brief current
     * @return
     */
    [[nodiscard]] auto current() const -> float;

    /**
     * @brief factor
     * @return
     */
    [[nodiscard]] auto factor() const -> float;

private:
    float m_mean { 0.0 };
    float m_std_deviation { 0.0 };
    float m_allowable_factor { 0.0 };
    std::chrono::steady_clock::time_point m_last { std::chrono::steady_clock::now() };
};

}

#endif // RATESUPERVISOR_H
