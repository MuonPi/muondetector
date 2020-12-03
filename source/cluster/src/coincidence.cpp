#include "coincidence.h"
#include "event.h"

#include <cmath>

#include <chrono>

namespace MuonPi {

Coincidence::~Coincidence() = default;

auto Coincidence::criterion(const std::unique_ptr<Event>& first, const std::unique_ptr<Event>& second) const -> float
{
    const std::chrono::steady_clock::time_point s1 {first->start()};
    const std::chrono::steady_clock::time_point s2 {second->start()};
    std::chrono::steady_clock::time_point e1 {first->start()};
    std::chrono::steady_clock::time_point e2 {second->start()};

    if (first->n() > 1) {
        e1 = first->end();
    }
    if (second->n() > 1) {
        e2 = second->end();
    }

    return compare(s1, s2) + compare(s1, e2) + compare(e1, s2) + compare(e1, e2);
}

auto Coincidence::compare(std::chrono::steady_clock::time_point t1, std::chrono::steady_clock::time_point t2) const -> float
{
    return (std::chrono::abs(t1 - t2) <= m_time)?1.0f:-1.0f;
}
}
