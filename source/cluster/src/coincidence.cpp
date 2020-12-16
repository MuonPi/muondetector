#include "coincidence.h"
#include "event.h"

#include <cmath>

#include <chrono>

namespace MuonPi {

Coincidence::~Coincidence() = default;

auto Coincidence::criterion(const Event &first, const Event &second) const -> float
{
    std::int_fast64_t d_epoch { (first.epoch() - second.epoch()) * static_cast<std::int_fast64_t>(1e9) };
    std::int_fast64_t t11 { first.start() };
    std::int_fast64_t t12 { (first.n() > 1)?first.end():first.start() };
    std::int_fast64_t t21 { second.start() };
    std::int_fast64_t t22 { (second.n() > 1)?second.end():second.start() };

    return compare(d_epoch + (t11 - t21)) + compare(d_epoch + (t11 - t22)) + compare(d_epoch + (t12 - t21)) + compare(d_epoch + (t12 - t22));
}

auto Coincidence::compare(int_fast64_t difference) const -> float
{
    return (std::abs(difference) <= m_time)?1.0f:-1.0f;
}
}
