#include "plausability.h"
#include "abstractevent.h"

namespace MuonPi {

Plausability::~Plausability() = default;

auto Plausability::criterion(const std::unique_ptr<AbstractEvent>& /*first*/, const std::unique_ptr<AbstractEvent>& /*second*/) const -> double
{
    return 0.0;
}

auto Plausability::met(const double& /*value*/) const -> bool
{
    return false;
}
}
