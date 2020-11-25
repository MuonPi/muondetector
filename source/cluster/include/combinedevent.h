#ifndef COMBINEDEVENT_H
#define COMBINEDEVENT_H

#include "abstractevent.h"

#include <memory>
#include <vector>

namespace MuonPi {

/**
 * @brief The CombinedEvent class
 * Holds a combined event. Usually is a coincidence.
 */
class CombinedEvent : public AbstractEvent
{
public:
    /**
     * @brief CombinedEvent
     * @param id The id of the event
     */
    CombinedEvent(std::uint64_t id, std::unique_ptr<AbstractEvent> event) noexcept;

    ~CombinedEvent() noexcept override;
    /**
     * @brief add_event Adds an event to the CombinedEvent.
     * @param event The event to add. In the case that the abstract event is a combined event, the child events will be added instead of their combination.
     * 				The unique_ptr will be moved.
     * @return false in the case of an invalid Event, true otherwise
     */
    [[nodiscard]] auto add_event(std::unique_ptr<AbstractEvent> event) noexcept -> bool;

    /**
     * @brief n The current number of events in the combination
     * @return The current number of events in this combined event
     */
    [[nodiscard]] auto n() const noexcept -> std::size_t;

    /**
      * @brief Get the list of events. Moves the vector.
      * @return The list of events contained in this combined event
      */
    [[nodiscard]] auto events() -> const std::vector<std::unique_ptr<AbstractEvent>>&;

    [[nodiscard]] auto time() const noexcept -> std::vector<std::chrono::steady_clock::time_point> override;
private:
    std::vector<std::unique_ptr<AbstractEvent>> m_events {};
};
}

#endif // COMBINEDEVENT_H
