#include "combinedevent.h"

namespace MuonPi {

auto CombinedEvent::add_event(std::unique_ptr<AbstractEvent> /*event*/) -> bool
{
    return false;
}

auto CombinedEvent::n() const -> std::size_t
{
    return 0;
}

}
