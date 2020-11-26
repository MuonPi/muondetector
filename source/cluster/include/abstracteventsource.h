#ifndef ABSTRACTEVENTSOURCE_H
#define ABSTRACTEVENTSOURCE_H

#include <future>
#include <memory>
#include <atomic>
#include <queue>

namespace MuonPi {

// +++ forward declarations
class Event;
// --- forward declarations

/**
 * @brief The AbstractEventSource class
 * Represents a canonical Source of events.
 */
class AbstractEventSource
{
public:
    /**
     * @brief AbstractEventSource The constructor. Initialises the Event source and starts the internal event loop.
     */
    AbstractEventSource();

    /**
     * @brief ~AbstractEventSource The destructor. If this gets called while the event loop is still running, it will tell the loop to finish and wait for it to be done.
     */
    virtual ~AbstractEventSource();

    /**
      * @brief next_event gets the next available Event from the internal event buffer.
      * Should the event buffer be empty, a promise is created which will be filled once a new event is availabale.
      * @return a future to the next available Event.
      */
    [[nodiscard]] auto next_event() -> std::future<std::unique_ptr<Event>> ;

    /**
     * @brief stop
     * Tells the event sink to stop its work at the next possible time
     */
    void stop();

protected:
    /**
      * @brief step method to be reimplemented by child classes. It is called from the event loop
      * @return Whether the step was successfully executed or not.
      * In the case of a return value false, the event loop stops.
      */
    [[nodiscard]] virtual auto step() -> bool;

    /**
     * @brief push_event pushes an Event into the event sink
     * @param event The event to push
     */
    void push_event(std::unique_ptr<Event> event);

private:
    /**
     * @brief run The event loop. This executes a loop asynchronuously until an exit condition is reached.
     * Gets called in the constructor automatically.
     */
    void run();
    std::atomic<bool> m_run { true };
    std::future<void> m_run_future {};

    std::promise<std::unique_ptr<Event>> m_event_promise {};
    std::atomic<bool> m_has_event_promise { false };

    std::queue<std::unique_ptr<Event>> m_event_queue {};

};

}

#endif // ABSTRACTEVENTSOURCE_H
