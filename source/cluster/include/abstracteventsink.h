#ifndef ABSTRACTEVENTSINK_H
#define ABSTRACTEVENTSINK_H

#include <future>
#include <memory>
#include <atomic>
#include <queue>

namespace MuonPi {

class AbstractEvent;

class AbstractEventSink
{
public:
    AbstractEventSink();

    virtual ~AbstractEventSink();

    void stop();

    void push_event(std::unique_ptr<AbstractEvent> event);

protected:
    [[nodiscard]] virtual auto step() -> bool;

    [[nodiscard]] auto next_event() -> std::future<std::unique_ptr<AbstractEvent>> ;

private:
    void run();
    std::atomic<bool> m_run { true };
    std::future<void> m_run_future {};

    std::promise<std::unique_ptr<AbstractEvent>> m_event_promise {};
    std::atomic<bool> m_has_event_promise { false };

    std::queue<std::unique_ptr<AbstractEvent>> m_event_queue {};

};

}

#endif // ABSTRACTEVENTSINK_H
