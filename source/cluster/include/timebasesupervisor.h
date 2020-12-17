#ifndef TIMEBASESUPERVISOR_H
#define TIMEBASESUPERVISOR_H


#include <memory>
#include <chrono>

namespace MuonPi {

class Event;

/**
 * @brief The TimeBaseSupervisor class
 */
class TimeBaseSupervisor
{
public:
    /**
     * @brief TimeBaseSupervisor
     * @param sample_time The sampling time of the Supervisor
     */
    TimeBaseSupervisor(std::chrono::system_clock::duration sample_time);

    /**
     * @brief process_event Handle one incoming event
     * @param event The event to check
     */
    void process_event(const Event& event);

    /**
     * @brief current get the current rate
     * @return The current time base of incoming events
     */
    [[nodiscard]] auto current() -> std::chrono::system_clock::duration;

private:
    static constexpr std::chrono::system_clock::duration s_minimum { std::chrono::milliseconds{800} };

    std::chrono::system_clock::duration m_sample_time { std::chrono::seconds{1} };
    std::chrono::system_clock::time_point m_sample_start { std::chrono::system_clock::now() };

    std::int_fast64_t m_epoch { std::chrono::system_clock::now().time_since_epoch().count() };
    std::int_fast64_t m_start { 1000000000 };
    std::int_fast64_t m_end { -1000000000 };


    std::chrono::system_clock::duration m_current { s_minimum };

};

}

#endif // TIMEBASESUPERVISOR_H
