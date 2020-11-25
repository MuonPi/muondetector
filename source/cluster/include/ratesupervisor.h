#ifndef RATESUPERVISOR_H
#define RATESUPERVISOR_H


#include <memory>

namespace MuonPi {

class AbstractEvent;


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
    RateSupervisor(float mean, float std_deviation);

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
    [[maybe_unused]] float m_mean { 0.0 };
    [[maybe_unused]] float m_std_deviation { 0.0 };
};

}

#endif // RATESUPERVISOR_H
