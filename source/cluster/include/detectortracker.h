#ifndef DETECTORTRACKER_H
#define DETECTORTRACKER_H

#include "threadrunner.h"

#include <map>
#include <memory>
#include <queue>


namespace MuonPi {

class Event;
class DetectorInfo;
class DetectorSummary;
template <typename T>
class AbstractSource;
template <typename T>
class AbstractSink;
class Detector;
class StateSupervisor;


class DetectorTracker : public ThreadRunner
{
public:
    /**
     * @brief DetectorTracker
     * @param log_sources A vector of log sources to use
     * @param log_sinks A vector of log sinks to write log items to
     * @param supervisor A reference to a supervisor object, which keeps track of program metadata
     */
    DetectorTracker(std::vector<std::shared_ptr<AbstractSource<DetectorInfo>>> log_sources, std::vector<std::shared_ptr<AbstractSink<DetectorSummary>>> log_sinks, StateSupervisor& supervisor);

    /**
     * @brief accept Check if an event is accepted
     * @param event The event to check
     * @return true if the event belongs to a known detector and the detector is reliable
     */
    [[nodiscard]] auto accept(Event &event) const -> bool;

    /**
     * @brief factor The current maximum factor
     * @return maximum factor between all detectors
     */
    [[nodiscard]] auto factor() const -> double;


    [[nodiscard]] auto get(std::size_t hash) const -> std::shared_ptr<Detector>;
protected:

    /**
     * @brief process Process a log message. Hands the message over to a detector, if none exists, creates a new one.
     * @param log The log message to check
     */
    void process(const DetectorInfo& log);

    /**
     * @brief step reimplemented from ThreadRunner
     * @return true if the step succeeded.
     */
    [[nodiscard]] auto step() -> int override;

private:
    StateSupervisor& m_supervisor;

    std::vector<std::shared_ptr<AbstractSource<DetectorInfo>>> m_log_sources {};
    std::vector<std::shared_ptr<AbstractSink<DetectorSummary>>> m_log_sinks {};

    std::map<std::size_t, std::shared_ptr<Detector>> m_detectors {};

    std::queue<std::size_t> m_delete_detectors {};

    double m_factor { 1.0 };

    std::chrono::steady_clock::time_point m_last { std::chrono::steady_clock::now() };
};

}

#endif // DETECTORTRACKER_H
