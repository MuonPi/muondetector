#ifndef RATESUPERVISOR_H
#define RATESUPERVISOR_H


#include "event.h"

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

    [[nodiscard]] auto average() -> float;
    [[nodiscard]] auto mean_average() -> float;

    static constexpr std::size_t s_history_length { 10 };

    std::size_t m_current_count { 0 };
    std::size_t m_current_index { 0 };
    std::size_t m_current_mean_index { 0 };
    std::size_t m_n { 0 };

    bool m_full { false };
    bool m_mean_full { false };

    float m_factor { 1.0f };
    Rate m_default {};
    Rate m_allowed {};
    Rate m_current {};
    std::array<float, s_history_length> m_mean_history {};
    std::array<float, s_history_length> m_history {};
    std::atomic<bool> m_dirty { false };
    std::chrono::system_clock::time_point m_last { std::chrono::system_clock::now() };
};

}

#endif // RATESUPERVISOR_H
