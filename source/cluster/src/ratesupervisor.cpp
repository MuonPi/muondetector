#include "ratesupervisor.h"

#include "log.h"

namespace MuonPi {

void RateSupervisor::tick(bool message)
{
    if (message) {
        m_current.increase_counter();
        m_mean.increase_counter();
    }


    if (m_current.step()) {
        m_mean.step();
        if (m_current.mean() < (m_mean.mean() - m_mean.deviation())) {
            double old = m_factor;
            m_factor = ((m_mean.mean() - m_current.mean())/(m_mean.deviation()) + 1.0 ) * 2.0;
            if (std::abs(m_factor - old) > 0.05) {
                mark_dirty();
            }
        } else {
            if (std::abs(m_factor - 1.0) > 0.05) {
                m_factor = 1.0;
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
