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

auto ThreadRunner::run() -> int
{
    Log::debug()<<"Starting thread " + m_name;
    int pre_result { pre_run() };
    m_started = true;
    if (pre_result != 0) {
        return pre_result;
    }
    while (m_run) {
        int result { step() };
        if (result != 0) {
            Log::debug()<<"Thread " + m_name + " Stopped.";
            return result;
        }
    }
    Log::debug()<<"Stopping thread " + m_name;
    return post_run();
}

void ThreadRunner::finish()
{
    stop();
    join();
}

void ThreadRunner::start()
{
    if (m_started) {
        return;
    }
    m_run_future = std::async(std::launch::async, &ThreadRunner::run, this);
}

}
