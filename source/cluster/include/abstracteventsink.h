#ifndef ABSTRACTEVENTSINK_H
#define ABSTRACTEVENTSINK_H

#include <future>
#include <memory>
#include <atomic>
#include <queue>

namespace MuonPi {

// +++ Forward declarations
class Event;
// --- Forward declarations

/**
 * @brief The AbstractEventSink class
 * Represents a canonical Sink for events.
 */
class AbstractEventSink
{
public:
    /**
     * @brief AbstractEventSink The constructor. Initialises the Event sink and starts the internal event loop.
     */
    AbstractEventSink();

    /**
     * @brief ~AbstractEventSink The destructor. If this gets called while the event loop is still running, it will tell the loop to finish and wait for it to be done.
     */
    virtual ~AbstractEventSink();

    /**
     * @brief stop
     * Tells the event sink to stop its work at the next possible time
     */
    void stop();

    /**
     * @brief push_event pushes an Event into the event sink
     * @param event The event to push
     */
    void push_event(std::unique_ptr<Event> event);

protected:
    /**
      * @brief step method to be reimplemented by child classes. It is called from the event loop
      * @return Whether the step was successfully executed or not.
      * In the case of a return value false, the event loop stops.
      */
    [[nodiscard]] virtual auto step() -> bool;

    /**
      * @brief next_event gets the next available Event from the internal event buffer.
      * Should the event buffer be empty, a promise is created which will be filled once a new event is availabale.
      * @return a future to the next available Event.
      */
    [[nodiscard]] auto next_event() -> std::future<std::unique_ptr<Event>>;

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

#endif // ABSTRACTEVENTSINK_H
