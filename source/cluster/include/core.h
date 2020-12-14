#ifndef CORE_H
#define CORE_H

#include "threadrunner.h"
#include "detector.h"
#include "coincidence.h"
#include "timebasesupervisor.h"
#include "eventconstructor.h"

#include <queue>
#include <map>


namespace MuonPi {

template <typename T>
class AbstractSink;
template <typename T>
class AbstractSource;
class AbstractDetectorTracker;

/**
 * @brief The Core class
 */
class Core : public ThreadRunner
{
public:
    Core(std::unique_ptr<AbstractSink<Event>> event_sink, std::unique_ptr<AbstractSource<Event>> event_source, std::unique_ptr<AbstractDetectorTracker> detector_tracker);

protected:
    /**
     * @brief step reimplemented from ThreadRunner
     * @return zero if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

    /**
     * @brief post_run reimplemented from ThreadRunner
     * @return zero in case of success.
     */
    [[nodiscard]] auto post_run() -> int override;



private:
    /**
     * @brief process Called from step(). Handles a new event arriving
     * @param event The event to process
     */
    void process(Event event);

    std::unique_ptr<AbstractSink<Event>> m_event_sink { nullptr };
    std::unique_ptr<AbstractSource<Event>> m_event_source { nullptr };
    std::unique_ptr<AbstractDetectorTracker> m_detector_tracker { nullptr };
    std::unique_ptr<TimeBaseSupervisor> m_time_base_supervisor { std::make_unique<TimeBaseSupervisor>( std::chrono::seconds{2} ) };

    std::shared_ptr<Criterion> m_criterion { std::make_shared<Coincidence>() };

    std::map<std::uint64_t, std::unique_ptr<EventConstructor>> m_constructors {};

    std::queue<std::uint64_t> m_delete_constructors {};

    std::chrono::system_clock::duration m_timeout { std::chrono::minutes{1} };

};

}

#endif // CORE_H
