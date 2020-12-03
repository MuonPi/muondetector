#ifndef THREADRUNNER_H
#define THREADRUNNER_H

#include <atomic>
#include <future>

namespace MuonPi {

/**
 * @brief The ThreadRunner class. Inherit from this to get a class which has an internal main loop.
 * If an instance of this class is created without inheriting from it, the main loop will stop immediatly per default.
 */
class ThreadRunner
{
public:
    /**
     * @brief ~ThreadRunner Stops the thread and waits for it to finish.
     */
    virtual ~ThreadRunner();

    /**
     * @brief stop Tells the main loop to finish
     */
    void stop();

    /**
     * @brief wait Wait for the main loop to finish
     * @return the return value of the main loop
     */
    [[nodiscard]] auto wait() -> bool;

protected:
    /**
     * @brief run executed as the main loop
     */
    [[nodiscard]] auto run() -> bool;

    /**
     * @brief finish Tells the main loop to finish and waits for the thread to exit
     */
    void finish();

    /**
     * @brief step executed each loop
     * @return false if the loop should stop immediatly, true otherwise
     */
    [[nodiscard]] virtual auto step() -> bool;

private:
    std::atomic<bool> m_run { true };

    std::future<bool> m_run_future { std::async(std::launch::async, &ThreadRunner::run, this) };
};

}

#endif // THREADRUNNER_H
