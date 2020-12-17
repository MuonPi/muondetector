#ifndef EVENTCONSTRUCTOR_H
#define EVENTCONSTRUCTOR_H

#include "event.h"

#include <memory>
#include <chrono>

namespace MuonPi {

class Event;
class Criterion;
/**
 * @brief The EventConstructor class
 */
class EventConstructor
{
public:
    enum class Type {
        NoMatch,
        Contested,
        Match
    };

    /**
     * @brief set_timeout Set a new timeout for the EventConstructor. Only accepts longer timeouts.
     * @param timeout The timeout to set.
     */
    void set_timeout(std::chrono::system_clock::duration timeout);

    /**
     * @brief timed_out Check whether the timeout has been reached
     * @return true if the constructor timed out.
     */
    [[nodiscard]] auto timed_out() const -> bool;

    Event event;
    std::chrono::system_clock::duration timeout { std::chrono::minutes{1} };

private:
    std::chrono::system_clock::time_point m_start { std::chrono::system_clock::now() };
};

}

#endif // EVENTCONSTRUCTOR_H
