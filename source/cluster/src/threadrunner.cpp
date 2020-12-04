#include "threadrunner.h"

namespace MuonPi {


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
    return m_run_future.get();
}

auto ThreadRunner::run() -> int
{
    if (m_run_future.valid()) {
        return -2;
    }
    int pre_result { pre_run() };
    if (pre_result != 0) {
        return pre_result;
    }
    while (m_run) {
        int result { step() };
        if (result != 0) {
            return result;
        }
    }
    return post_run();
}

void ThreadRunner::finish()
{
    stop();
    join();
}

}
