#ifndef COMBINEDEVENT_H
#define COMBINEDEVENT_H

#include "event.h"

#include <memory>
#include <vector>

namespace MuonPi {

/**
 * @brief The CombinedEvent class
 * Holds a combined event. Usually is a coincidence.
 */
class CombinedEvent : public Event
{
public:
    /**
     * @brief CombinedEvent
     * @param id The id of the event
     */
    CombinedEvent(std::uint64_t id, std::unique_ptr<Event> event) noexcept;

    ~CombinedEvent() noexcept override;
    /**
     * @brief add_event Adds an event to the CombinedEvent.
     * @param event The event to add. In the case that the abstract event is a combined event, the child events will be added instead of their combination.
     * 				The unique_ptr will be moved.
     */
    void add_event(std::unique_ptr<Event> event) noexcept;

    /**
     * @brief mark_contested Mark this combined event as containing a set of events which have at least one possible subset of events though does not work as an event in its totality.
     */
    void mark_contested();

    /**
     * @brief contested
     * @return Return whether this event is contested or not.
     * @see mark_contested
     */
    [[nodiscard]] auto contested() const -> bool;

    /**
      * @brief Get the list of events. Moves the vector.
      * @return The list of events contained in this combined event as const ref
      */
    [[nodiscard]] auto events_ref() const -> const std::vector<std::unique_ptr<Event>>&;

    /**
      * @brief Get the list of events. Moves the vector.
      * @return The list of events contained in this combined event
      */
    [[nodiscard]] auto events() -> std::vector<std::unique_ptr<Event>>;

private:
    std::vector<std::unique_ptr<Event>> m_events {};
    bool m_contested { false };

};
}

#endif // COMBINEDEVENT_H
