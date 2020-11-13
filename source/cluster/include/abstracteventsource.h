#ifndef ABSTRACTEVENTSOURCE_H
#define ABSTRACTEVENTSOURCE_H

#include <future>
#include <memory>
#include <atomic>
#include <queue>

namespace MuonPi {

class AbstractEvent;

class AbstractEventSource
{
public:
    AbstractEventSource();

    virtual ~AbstractEventSource();

    [[nodiscard]] auto next_event() -> std::future<std::unique_ptr<AbstractEvent>> ;

    void stop();

protected:
    [[nodiscard]] virtual auto step() -> bool;
    void push_event(std::unique_ptr<AbstractEvent> event);

private:
    void run();
    std::atomic<bool> m_run { true };
    std::future<void> m_run_future {};

    std::promise<std::unique_ptr<AbstractEvent>> m_event_promise {};
    std::atomic<bool> m_has_event_promise { false };

    std::queue<std::unique_ptr<AbstractEvent>> m_event_queue {};

};

}

#endif // ABSTRACTEVENTSOURCE_H
