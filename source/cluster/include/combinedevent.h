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
     * @brief add_event Adds an event to the CombinedEvent.
     * @param event The event to add. In the case that the abstract event is a combined event, the child events will be added instead of their combination.
     * 				The unique_ptr will be moved.
     * @return false in the case of an invalid Event, true otherwise
     */
    [[nodiscard]] auto add_event(std::unique_ptr<AbstractEvent> event) -> bool;

    /**
     * @brief n The current number of events in the combination
     * @return The current number of events in this combined event
     */
    [[nodiscard]] auto n() const -> std::size_t;

private:
    std::vector<std::unique_ptr<AbstractEvent>> m_events {};
};
}

#endif // COMBINEDEVENT_H
