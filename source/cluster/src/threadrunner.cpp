#include "threadrunner.h"

#include "log.h"

namespace MuonPi {

ThreadRunner::ThreadRunner(const std::string& name)
    : m_name { name }
{}

ThreadRunner::~ThreadRunner()
{
    finish();
}

void ThreadRunner::stop()
{
    m_run = false;
}

void ThreadRunner::join()
{
    if (m_run_future.valid()) {
        m_run_future.wait();
    }
}

auto ThreadRunner::step() -> int
{
    return 0;
}

auto ThreadRunner::pre_run() -> int
{
    return 0;
}

auto ThreadRunner::post_run() -> int
{
    return 0;
}

auto ThreadRunner::wait() -> int
{
    if (!m_run_future.valid()) {
        return -1;
    }
    join();
    return m_run_future.get();
}

auto ThreadRunner::state() -> State
{
    return m_state;
}

auto ThreadRunner::run() -> int
{
    m_state = State::Initialising;
    struct StateGuard {
        State& state;
        bool clean { false };

        ~StateGuard()
        {
            if (clean) {
                state = State::Stopped;
            } else {
                state = State::Error;
            }
        }
    } guard {m_state};

    Log::debug()<<"Starting thread " + m_name;
    int pre_result { pre_run() };
    if (pre_result != 0) {
        return pre_result;
    }
    try {
    m_state = State::Running;
    while (m_run) {
        int result { step() };
        if (result != 0) {
            Log::debug()<<"Thread " + m_name + " Stopped.";
            return result;
        }
    }
    } catch (std::exception& e) {
        Log::error()<<"Thread " + m_name + "Got an uncaught exception: " + std::string{e.what()};
        return -1;
    } catch (...) {
        Log::error()<<"Thread " + m_name + "Got an uncaught exception.";
        return -1;
    }
    m_state = State::Finalising;
    Log::debug()<<"Stopping thread " + m_name;
    guard.clean = true;
    return post_run();
}

void ThreadRunner::finish()
{
    stop();
    join();
}

void ThreadRunner::start()
{
    if (m_state > State::Stopped) {
        return;
    }
    m_run_future = std::async(std::launch::async, &ThreadRunner::run, this);
}

}
