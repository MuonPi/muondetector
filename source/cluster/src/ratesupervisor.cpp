#include "ratesupervisor.h"

#include "log.h"

namespace MuonPi {

void RateSupervisor::tick(bool message)
{
    if (message) {
        m_measurement.increase_counter();
        m_mean.increase_counter();
    }


    if (m_measurement.step()) {
        m_mean.step();
        if (m_measurement.mean() < (m_mean.mean() - m_mean.deviation()*3.0)) {
            double old = m_factor;
            m_factor = (m_measurement.mean() - m_mean.mean())/(m_mean.deviation()*3.0);
            if (std::abs(m_factor - old) > 0.05) {
                mark_dirty();
            }
        } else {
            double old = m_factor;
            m_factor = 1.0;
            if (std::abs(m_factor - old) > 0.05) {
                mark_dirty();
            }
        }
    }
}

auto RateSupervisor::factor() const -> double
{
    return m_factor;
}

auto RateSupervisor::dirty() -> bool
{
    if (m_dirty) {
        m_dirty = false;
        return true;
    }
    return false;
}


void RateSupervisor::mark_dirty()
{
    m_dirty = true;
}

}
