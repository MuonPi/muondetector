#include "coincidence.h"

namespace MuonPi {

Coincidence::~Coincidence() = default;

auto Coincidence::criterion(const std::unique_ptr<AbstractEvent>& /*first*/, const std::unique_ptr<AbstractEvent>& /*second*/) const -> bool
{
    return false;
}

auto Coincidence::met(const bool& /*value*/) const -> bool
{
    return false;
}
}
