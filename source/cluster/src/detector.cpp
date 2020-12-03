#include "detector.h"
#include "ratesupervisor.h"
#include "event.h"

namespace MuonPi {

Detector::Listener::~Listener() = default;

Detector::Detector(Listener* listener, const LogMessage &initial_log)
    : ThreadRunner {}
    , m_location { initial_log.location()}
    , m_hash { initial_log.hash() }
    , m_listener { listener }
    , m_supervisor { std::make_unique<RateSupervisor>(RateSupervisor::Rate{}) }
{
}

Detector::~Detector()
{
    ThreadRunner::~ThreadRunner();
}

auto Detector::process(const Event& event) -> bool
{
    if (event.hash() != m_hash) {
        return false;
    }

    m_tick = true;

    return true;
}

auto Detector::process(const LogMessage &log) -> bool
{
    if (log.hash() != m_hash) {
        return false;
    }

    m_last_log = std::chrono::steady_clock::now();

    m_location = log.location();

    return true;
}

void Detector::set_status(Status status)
{
    if (status != m_status) {
        m_status = status;
        m_listener->detector_status_changed(m_hash, status);
    }
}

auto Detector::is(Status status) const -> bool
{
    return m_status == status;
}

auto Detector::step() -> bool
{
    auto diff { std::chrono::steady_clock::now() - std::chrono::steady_clock::time_point { m_last_log } };
    if (diff > s_log_interval) {
        if (diff > s_wait_interval) {
            set_status(Status::Quitting);
        } else {
            set_status(Status::Unreliable);
        }
    } else {
        if ((m_location.prec > Location::minimum_prec) || (m_location.iop < Location::maximum_iop)) {
            set_status(Status::Unreliable);
        } else {
            set_status(Status::Reliable);
        }
    }
    if (is(Status::Reliable)) {
        m_supervisor->tick(m_tick);
        if (m_tick) {
            m_tick = false;
        }
        if (m_supervisor->dirty()) {
            m_listener->factor_changed(m_hash, m_supervisor->factor());
        }
    }
    std::this_thread::sleep_for( std::chrono::milliseconds{10} );

    return true;
}

}
