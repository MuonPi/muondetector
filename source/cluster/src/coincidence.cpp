#include "coincidence.h"
#include "abstractevent.h"

namespace MuonPi {

Coincidence::~Coincidence() = default;

auto Coincidence::criterion(const std::unique_ptr<AbstractEvent>& first, const std::unique_ptr<AbstractEvent>& second) const -> bool
{
    for (const auto& t1 : first->time()) {
        for (const auto& t2 : second->time()) {
            if (std::abs(std::chrono::duration_cast<std::chrono::microseconds>(t1 - t2).count()) > std::chrono::duration_cast<std::chrono::microseconds>(m_time).count()) {
                return false;
            }
        }
    }
    return true;
}
}
