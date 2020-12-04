#include "ratesupervisor.h"

namespace MuonPi {

RateSupervisor::RateSupervisor(Rate allowable)
    : m_allowed { allowable }
{

}

void RateSupervisor::tick(bool /*message*/)
{

}

auto RateSupervisor::current() const -> RateSupervisor::Rate
{
    return m_current;
}

auto RateSupervisor::factor() const -> float
{
    return {};
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
