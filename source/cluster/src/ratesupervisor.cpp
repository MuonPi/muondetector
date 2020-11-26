#include "ratesupervisor.h"

namespace MuonPi {

RateSupervisor::RateSupervisor(Rate /*allowable*/)
{

}

void RateSupervisor::tick(bool /*message*/)
{

}

auto RateSupervisor::current() const -> RateSupervisor::Rate
{
    return {};
}

}
