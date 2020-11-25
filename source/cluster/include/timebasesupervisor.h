#ifndef TIMEBASESUPERVISOR_H
#define TIMEBASESUPERVISOR_H


#include <memory>
#include <chrono>

namespace MuonPi {

class AbstractEvent;

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
    void process_event(const std::unique_ptr<AbstractEvent>& event);

    /**
     * @brief current
     * @return
     */
    [[nodiscard]] auto current() const -> std::chrono::steady_clock::duration;

private:
    [[maybe_unused]] std::chrono::steady_clock::time_point m_start { std::chrono::steady_clock::now() };
    [[maybe_unused]] std::chrono::steady_clock::time_point m_earliest {};
    [[maybe_unused]] std::chrono::steady_clock::time_point m_latest {};
    [[maybe_unused]] std::size_t m_n { 0 };
    [[maybe_unused]] std::chrono::steady_clock::duration m_current {};

};

}

#endif // TIMEBASESUPERVISOR_H
