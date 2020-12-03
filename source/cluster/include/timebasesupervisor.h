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
     * @brief restart
     * @return
     */
    [[nodiscard]] auto restart() -> std::chrono::steady_clock::duration;

    /**
     * @brief process_event
     * @param event
     */
    void process_event(const std::unique_ptr<Event>& event);

    /**
     * @brief current
     * @return
     */
    [[nodiscard]] auto current() const -> std::chrono::steady_clock::duration;

private:
    std::chrono::steady_clock::time_point m_start { std::chrono::steady_clock::now() };
    std::chrono::steady_clock::time_point m_earliest {std::chrono::steady_clock::now() + std::chrono::hours{24}};
    std::chrono::steady_clock::time_point m_latest {std::chrono::steady_clock::now() - std::chrono::hours{24}};
    std::size_t m_n { 0 };
    std::chrono::steady_clock::duration m_current {};

};

}

#endif // TIMEBASESUPERVISOR_H
