#ifndef CORE_H
#define CORE_H

#include "threadrunner.h"
#include "detector.h"

#include <queue>
#include <map>


namespace MuonPi {

template <typename T>
class AbstractSink;
template <typename T>
class AbstractSource;
class Criterion;
class EventConstructor;
class TimeBaseSupervisor;

class Core : public Detector::Listener, public ThreadRunner
{
public:
    Core();
    ~Core() override;


    void factor_changed(std::size_t hash, float factor) override;
    void detector_status_changed(std::size_t hash, Detector::Status status) override;

protected:
    [[nodiscard]] auto step() -> bool override;

private:
    void handle_event(std::unique_ptr<Event> event);
    void handle_log(std::unique_ptr<LogMessage> log);

    std::unique_ptr<AbstractSink<Event>> m_event_sink { nullptr };
    std::unique_ptr<AbstractSource<Event>> m_event_source { nullptr };
    std::unique_ptr<AbstractSource<LogMessage>> m_log_source { nullptr };

    std::unique_ptr<Criterion> m_criterion { nullptr };

    std::unique_ptr<TimeBaseSupervisor> m_time_base_supervisor { nullptr };

    std::queue<std::unique_ptr<EventConstructor>> m_constructors {};

    std::map<std::size_t, std::unique_ptr<Detector>> m_detectors {};


    float m_factor { 1.0 };
};

}

#endif // CORE_H
