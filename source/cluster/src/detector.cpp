#include "detector.h"
#include "event.h"
#include "log.h"
#include "statesupervisor.h"

namespace MuonPi {

Detector::Detector(const DetectorInfo &initial_log, StateSupervisor& supervisor)
    : m_location { initial_log.location()}
    , m_hash { initial_log.hash() }
    , m_state_supervisor { supervisor }
{
}

void Detector::process(const Event& event)
{
    m_current_rate.increase_counter();
    m_mean_rate.increase_counter();

	double pulselength = event.data().end - event.data().start;
	if (pulselength > 0. && pulselength < 1e6) m_pulselength.add(pulselength);
}

void Detector::process(const DetectorInfo &info)
{
    m_last_log = std::chrono::system_clock::now();

    m_location = info.location();

    if ((m_location.prec > Location::maximum_prec) || (m_location.dop > Location::maximum_dop)) {
        set_status(Status::Unreliable);
    } else {
        set_status(Status::Reliable);
    }
}

void Detector::set_status(Status status)
{
    if (m_status != status) {
        m_state_supervisor.detector_status(m_hash, status);
    }
    m_status = status;
}

auto Detector::is(Status status) const -> bool
{
    return m_status == status;
}

auto Detector::factor() const -> double
{
    return m_factor;
}

auto Detector::step() -> bool
{
    auto diff { std::chrono::system_clock::now() - std::chrono::system_clock::time_point { m_last_log } };
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

    if (m_current_rate.step()) {
        m_mean_rate.step();
        if (m_current_rate.mean() < (m_mean_rate.mean() - m_mean_rate.deviation())) {
            m_factor = ((m_mean_rate.mean() - m_current_rate.mean())/(m_mean_rate.deviation()) + 1.0 ) * 2.0;
        } else {
            m_factor = 1.0;
        }
    }

    return true;
}

auto Detector::current_log_data() -> DetectorLog
{
	m_current_data.mean_eventrate = m_current_rate.mean();
	m_current_data.mean_pulselength = m_pulselength.mean();
	DetectorLog log(m_hash, m_current_data);
	m_current_data.incoming = 0;
	return log;
}

}
