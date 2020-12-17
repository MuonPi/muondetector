#ifndef CORE_H
#define CORE_H

#include "threadrunner.h"
#include "detector.h"
#include "coincidence.h"
#include "timebasesupervisor.h"
#include "eventconstructor.h"
#include "statesupervisor.h"
#include "clusterlog.h"

#include <queue>
#include <map>
#include <vector>


namespace MuonPi {

template <typename T>
class AbstractSink;
template <typename T>
class AbstractSource;
class DetectorTracker;

/**
 * @brief The Core class
 */
class Core : public ThreadRunner
{
public:
    Core(std::vector<std::shared_ptr<AbstractSink<Event>>> event_sinks, std::vector<std::shared_ptr<AbstractSource<Event>>> event_sources, DetectorTracker& detector_tracker, StateSupervisor& supervisor);

    [[nodiscard]] auto supervisor() -> StateSupervisor&;

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

    void push_event(Event event);

    std::vector<std::shared_ptr<AbstractSink<Event>>> m_event_sinks;
    std::vector<std::shared_ptr<AbstractSource<Event>>> m_event_sources;

    DetectorTracker& m_detector_tracker;
    std::unique_ptr<TimeBaseSupervisor> m_time_base_supervisor { std::make_unique<TimeBaseSupervisor>( std::chrono::seconds{2} ) };

    std::shared_ptr<Criterion> m_criterion { std::make_shared<Coincidence>() };

    std::vector<std::unique_ptr<EventConstructor>> m_constructors {};

    std::chrono::system_clock::duration m_timeout { std::chrono::minutes{1} };

    StateSupervisor& m_supervisor;


    double m_scale { 1.0 };


};

}

#endif // CORE_H
