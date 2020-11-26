#include "timebasesupervisor.h"
#include "event.h"

namespace MuonPi {
auto TimeBaseSupervisor::restart() -> std::chrono::steady_clock::duration
{
    return {};
}

void TimeBaseSupervisor::process_event(const std::unique_ptr<Event>& /*event*/)
{

}

auto TimeBaseSupervisor::current() const -> std::chrono::steady_clock::duration
{
    return {};
}
}
