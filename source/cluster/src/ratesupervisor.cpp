#include "ratesupervisor.h"

namespace MuonPi {

RateSupervisor::RateSupervisor(float /*mean*/, float /*std_deviation*/)
{

}

void RateSupervisor::tick(bool /*message*/)
{

}

auto RateSupervisor::current() const -> float
{
    return {};
}

auto RateSupervisor::factor() const -> float
{
    return {};
}

}
