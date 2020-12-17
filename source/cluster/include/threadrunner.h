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
    enum class State {
        Error,
        Stopped,
        Initial,
        Initialising,
        Running,
        Finalising
    };
    ThreadRunner(const std::string& name);
    /**
     * @brief ~ThreadRunner Stops the thread and waits for it to finish.
     */
    virtual ~ThreadRunner();

    /**
     * @brief stop Tells the main loop to finish
     */
    void stop();

    /**
     * @brief join waits for the thread to finish
     */
    void join();

    /**
     * @brief wait Wait for the main loop to finish
     * @return the return value of the main loop
     */
    [[nodiscard]] auto wait() -> int;

    /**
     * @brief state Indicates the current state of the thread
     * @return The current state
     */
    [[nodiscard]] auto state() -> State;

    /**
     * @brief name The name of the thread
     * @return the name
     */
    [[nodiscard]] auto name() -> std::string;

    /**
     * @brief state_string The current state of the thread
     * @return The current state of the thread but represented as a string
     */
    [[nodiscard]] auto state_string() -> std::string;

    /**
     * @brief start Starts the thread asynchronuosly
     */
    void start();

    /**
     * @brief start_synchronuos Starts the thread synchronuosly
     */
    void start_synchronuos();

protected:
    /**
     * @brief run executed as the main loop
     */
    [[nodiscard]] auto run() -> int;

    /**
     * @brief finish Tells the main loop to finish and waits for the thread to exit
     */
    void finish();

    /**
     * @brief step executed each loop
     * @return false if the loop should stop immediatly, true otherwise
     */
    [[nodiscard]] virtual auto step() -> int;

    /**
     * @brief pre_run Executed before the thread loop starts
     * @return Thread loop will not start with a nonzero return value
     */
    [[nodiscard]] virtual auto pre_run() -> int;

    /**
     * @brief post_run Executed after the thread loop stops
     * @return The return value will be returned from the thread loop
     */
    [[nodiscard]] virtual auto post_run() -> int;


private:
    std::atomic<bool> m_run { true };

    std::future<int> m_run_future {};

    std::string m_name {};

    State m_state { State::Initial };
};

}

#endif // THREADRUNNER_H
