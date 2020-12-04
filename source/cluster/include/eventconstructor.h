#ifndef EVENTCONSTRUCTOR_H
#define EVENTCONSTRUCTOR_H


#include <memory>
#include <chrono>

namespace MuonPi {

class Event;
class CombinedEvent;
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
     * @brief EventConstructor
     * @param event The initial event from which the constructor stems
     * @param criterion The criterion to use to check events
     * @param timeout the initial timeout to use
     */
    EventConstructor(std::unique_ptr<Event> event, std::shared_ptr<Criterion> criterion, std::chrono::steady_clock::duration timeout);

    ~EventConstructor();

    /**
     * @brief add_event
     * @param event The event to add
     * @param contested Whether the event is contested or not
     */
    void add_event(std::unique_ptr<Event> event, bool contested = false);

    /**
     * @brief event_matches Check whether an event matches the criterion
     * @param event The event to check
     * @return The type of match
     */
    [[nodiscard]] auto event_matches(const Event& event) -> Type;

    /**
     * @brief set_timeout Set a new timeout for the EventConstructor. Only accepts longer timeouts.
     * @param timeout The timeout to set.
     */
    void set_timeout(std::chrono::steady_clock::duration timeout);

    /**
     * @brief commit Hand over the event unique_ptr
     * @return The unique_ptr to the constructed event
     */
    [[nodiscard]] auto commit() -> std::unique_ptr<Event>;

    /**
     * @brief timed_out Check whether the timeout has been reached
     * @return true if the constructor timed out.
     */
    [[nodiscard]] auto timed_out() const -> bool;

private:
    std::chrono::steady_clock::time_point m_start { std::chrono::steady_clock::now() };
    std::unique_ptr<CombinedEvent> m_event { nullptr };
    std::shared_ptr<Criterion> m_criterion { nullptr };
    std::chrono::steady_clock::duration m_timeout { std::chrono::minutes{1} };
};

}

#endif // EVENTCONSTRUCTOR_H
