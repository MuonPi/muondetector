#include "combinedevent.h"

namespace MuonPi {


CombinedEvent::CombinedEvent(std::uint64_t id) noexcept
    : AbstractEvent{id, {}}
{}

CombinedEvent::~CombinedEvent() noexcept = default;

auto CombinedEvent::add_event(std::unique_ptr<AbstractEvent> /*event*/) noexcept -> bool
{
    return false;
}

auto CombinedEvent::n() const -> std::size_t
{
    return 0;
}

}
