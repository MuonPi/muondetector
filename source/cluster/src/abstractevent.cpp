#include "abstractevent.h"

namespace MuonPi {

auto AbstractEvent::time() const -> std::chrono::steady_clock::time_point
{
    return m_time;
}

auto AbstractEvent::id() const -> std::uint64_t
{
    return m_id;
}

}
