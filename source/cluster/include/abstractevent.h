#ifndef ABSTRACTEVENT_H
#define ABSTRACTEVENT_H

#include <chrono>

namespace MuonPi {

/**
 * @brief The AbstractEvent class
 * Abstract class, which is used as an interface for single events and combined events
 */
class AbstractEvent
{
public:
    virtual ~AbstractEvent();
    /**
     * @brief time
     * @return The time when the event happened.
     */
    [[nodiscard]] auto time() const -> std::chrono::steady_clock::time_point;

    /**
     * @brief id
     * @return The id of this event
     */
    [[nodiscard]] auto id() const -> std::uint64_t;

private:
    std::uint64_t m_id {};
    std::chrono::steady_clock::time_point m_time {};
};
}

#endif // ABSTRACTEVENT_H
