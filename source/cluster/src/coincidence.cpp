#include "coincidence.h"
#include "event.h"

#include <cmath>

#include <chrono>

namespace MuonPi {

Coincidence::~Coincidence() = default;

auto Coincidence::criterion(const Event &first, const Event &second) const -> double
{
    const std::int_fast64_t t11 = first.start();
    const std::int_fast64_t t21 = second.start();

    const std::int_fast64_t t12 = (first.n() > 1)?first.end():first.start();
    const std::int_fast64_t t22 = (second.n() > 1)?second.end():second.start();

    return compare(t11, t21) + compare(t11, t22) + compare(t12, t21) + compare(t12, t22);
}

auto Coincidence::compare(std::int_fast64_t t1, std::int_fast64_t t2) const -> double
{
    return (std::abs(t1 - t2) <= m_time)?1.0:-1.0;
}
}
