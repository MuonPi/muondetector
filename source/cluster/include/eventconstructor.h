#ifndef EVENTCONSTRUCTOR_H
#define EVENTCONSTRUCTOR_H


#include <memory>
#include <chrono>

namespace MuonPi {

class AbstractEvent;
class CombinedEvent;
class Criterion;
/**
 * @brief The EventConstructor class
 */
class EventConstructor
{
public:
    /**
     * @brief EventConstructor
     * @param event
     * @param criterion
     */
    EventConstructor(std::unique_ptr<AbstractEvent> event, std::shared_ptr<Criterion> criterion);
    ~EventConstructor();
    /**
     * @brief add_event
     * @param event
     */
    void add_event(std::unique_ptr<AbstractEvent> event);
    /**
     * @brief set_timeout
     * @param timeout
     */
    void set_timeout(std::chrono::steady_clock::duration timeout);

    /**
     * @brief event_fits
     * @param event
     * @return
     */
    [[nodiscard]] auto event_fits(std::unique_ptr<AbstractEvent> event) -> bool;
    /**
     * @brief commit
     * @return
     */
    [[nodiscard]] auto commit() -> std::unique_ptr<AbstractEvent>;
    /**
     * @brief timed_out
     * @return
     */
    [[nodiscard]] auto timed_out() const -> bool;

private:
    [[maybe_unused]] std::chrono::steady_clock::duration m_timeout {};
    [[maybe_unused]] std::chrono::steady_clock::time_point m_start { std::chrono::steady_clock::now() };
    [[maybe_unused]] std::unique_ptr<CombinedEvent> m_event { nullptr };
    [[maybe_unused]] std::shared_ptr<Criterion> m_criterion { nullptr };
};

}

#endif // EVENTCONSTRUCTOR_H
