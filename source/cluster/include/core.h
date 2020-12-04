#ifndef CORE_H
#define CORE_H

#include "threadrunner.h"
#include "detector.h"
#include "coincidence.h"
#include "timebasesupervisor.h"

#include <queue>
#include <map>


namespace MuonPi {

template <typename T>
class AbstractSink;
template <typename T>
class AbstractSource;
class EventConstructor;
class DetectorTracker;

/**
 * @brief The Core class
 */
class Core : public ThreadRunner
{
public:
    Core(std::unique_ptr<AbstractSink<Event>> event_sink, std::unique_ptr<AbstractSource<Event>> event_source, std::unique_ptr<DetectorTracker> detector_tracker);

    ~Core() override;

protected:
    /**
     * @brief step reimplemented from ThreadRunner
     * @return true if the step succeeded.
     */
    [[nodiscard]] auto step() -> bool override;

private:
    /**
     * @brief process Called from step(). Handles a new event arriving
     * @param event The event to process
     */
    void process(std::unique_ptr<Event> event);

    std::unique_ptr<AbstractSink<Event>> m_event_sink { nullptr };
    std::unique_ptr<AbstractSource<Event>> m_event_source { nullptr };
    std::unique_ptr<DetectorTracker> m_detector_tracker { nullptr };
    std::unique_ptr<TimeBaseSupervisor> m_time_base_supervisor { std::make_unique<TimeBaseSupervisor>( std::chrono::seconds{2} ) };

    std::unique_ptr<Criterion> m_criterion { std::make_unique<Coincidence>() };

    std::map<std::uint64_t, std::unique_ptr<EventConstructor>> m_constructors {};

    std::queue<std::uint64_t> m_delete_constructors {};

    std::chrono::steady_clock::duration m_timeout { std::chrono::minutes{1} };

};

}

#endif // CORE_H
