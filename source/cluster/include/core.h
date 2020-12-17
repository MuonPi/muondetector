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
    /**
     * @brief Core
     * @param event_sinks A vector of event sinks to use
     * @param event_sources A vector of event sources to use
     * @param detector_tracker A reference to the detector tracker which keeps track of connected detectors
     * @param supervisor A reference to a StateSupervisor, which keeps track of program metadata
     */
    Core(std::vector<std::shared_ptr<AbstractSink<Event>>> event_sinks, std::vector<std::shared_ptr<AbstractSource<Event>>> event_sources, DetectorTracker& detector_tracker, StateSupervisor& supervisor);

    /**
     * @brief supervisor Acceess the supervision object
     */
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

    /**
     * @brief push_event Pushes an event in to the event sinks
     * @param event The event to push
     */
    void push_event(Event event);

    std::vector<std::shared_ptr<AbstractSink<Event>>> m_event_sinks;
    std::vector<std::shared_ptr<AbstractSource<Event>>> m_event_sources;

    DetectorTracker& m_detector_tracker;
    std::unique_ptr<TimeBaseSupervisor> m_time_base_supervisor { std::make_unique<TimeBaseSupervisor>( std::chrono::seconds{2} ) };

    std::unique_ptr<Criterion> m_criterion { std::make_unique<Coincidence>() };

    std::vector<EventConstructor> m_constructors {};

    std::chrono::system_clock::duration m_timeout { std::chrono::minutes{1} };

    StateSupervisor& m_supervisor;


    double m_scale { 1.0 };


};

}

#endif // CORE_H
