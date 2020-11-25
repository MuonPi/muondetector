#ifndef SINGLEEVENT_H
#define SINGLEEVENT_H

#include "abstractevent.h"

#include <string>

namespace MuonPi {
/**
 * @brief The SingleEvent class
 * It holds a single event, which originates from an individual Detector.
 */
class SingleEvent : public AbstractEvent
{
public:
    /**
     * @brief SingleEvent
     * @param hash a hash of the username and detector site_id
     * @param id The id of the event
     * @param time The time of the event
     * @param duration The time difference between falling and rising edge time
     */
    SingleEvent(std::size_t hash, std::uint64_t id, std::chrono::steady_clock::time_point time, std::chrono::steady_clock::duration duration) noexcept;

    /**
     * @brief SingleEvent
     * @param hash a hash of the username and detector site_id
     * @param id The id of the event
     * @param rising The time point of the rising edge time
     * @param falling The time point of the falling edge time
     */
    SingleEvent(std::size_t hash, std::uint64_t id, std::chrono::steady_clock::time_point rising, std::chrono::steady_clock::time_point falling) noexcept;

    ~SingleEvent() noexcept override;

    /**
      * @brief duration
      * @return The time difference between the falling and rising edge time
      */
    [[nodiscard]] auto duration() const -> std::chrono::steady_clock::duration;

private:
    std::chrono::steady_clock::duration m_duration {};

};
}

#endif // SINGLEEVENT_H
