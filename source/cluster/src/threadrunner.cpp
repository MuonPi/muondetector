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

auto ThreadRunner::step() -> bool
{
    return false;
}

auto ThreadRunner::wait() -> bool
{
    if (!m_run_future.valid()) {
        return false;
    }
    return m_run_future.get();
}

auto ThreadRunner::run() -> bool
{
    if (m_run_future.valid()) {
        return false;
    }
    while (m_run) {
        if (!step()) {
            return false;
        }
    }
    return true;
}

void ThreadRunner::finish()
{
    stop();
    if (m_run_future.valid()) {
        m_run_future.wait();
    }
}

}
