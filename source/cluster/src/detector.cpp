#include "detector.h"
#include "event.h"
#include "log.h"

namespace MuonPi {

Detector::Detector(const LogMessage &initial_log)
    : m_location { initial_log.location()}
    , m_hash { initial_log.hash() }
    , m_supervisor { std::make_unique<RateSupervisor>(RateSupervisor::Rate{}) }
{
}

void Detector::process(const Event& /*event*/)
{
    m_tick = true;
}

void Detector::process(const LogMessage &log)
{
    m_last_log = std::chrono::steady_clock::now();

    m_location = log.location();

    if ((m_location.prec > Location::maximum_prec) || (m_location.dop > Location::maximum_dop)) {
        set_status(Status::Unreliable);
    } else {
        set_status(Status::Reliable);
    }
}

void Detector::set_status(Status status)
{
    if (m_status != status) {
        Log::notice()<<"Marking detector " + std::to_string(m_hash) + ((status == Status::Reliable)?"Reliable":"Unreliable");
    }
    m_status = status;
}

auto Detector::is(Status status) const -> bool
{
    return m_status == status;
}

auto Detector::factor() const -> float
{
    return m_supervisor->factor();
}

auto Detector::step() -> bool
{
    auto diff { std::chrono::steady_clock::now() - std::chrono::steady_clock::time_point { m_last_log } };
    if (diff > s_log_interval) {
        if (diff > s_quit_interval) {
            return false;
        } else {
            set_status(Status::Unreliable);
        }
    } else {
        if ((m_location.prec > Location::maximum_prec) || (m_location.dop > Location::maximum_dop)) {
            set_status(Status::Unreliable);
        } else {
            set_status(Status::Reliable);
        }
    }
    m_supervisor->tick(m_tick);
    if (m_tick) {
        m_tick = false;
    }

    return true;
}

}
