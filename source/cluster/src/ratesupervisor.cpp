#include "ratesupervisor.h"

#include "log.h"

namespace MuonPi {

RateSupervisor::RateSupervisor(Rate allowable)
    : m_default { allowable }
    , m_allowed { allowable }
{

}

void RateSupervisor::tick(bool message)
{
    if (message) {
        m_current_count++;
    }
    if ((std::chrono::system_clock::now() - m_last) > std::chrono::seconds{2}) {
        m_history[m_current_index++] = static_cast<float>(m_current_count) * 0.5f;
        if (m_current_index >= s_history_length) {
            m_current_index = 0;
            m_full = true;
        }
        m_current.m = average();
        m_current.n = (m_allowed.m - m_current.m) / m_allowed.s;
        if (m_current.n < m_allowed.n) {
            if (std::abs(m_factor - 1.0f) > 0.05f) {
                m_factor = 1.0;
                mark_dirty();
            }
        } else {
            float old = m_factor;
            m_factor = (m_current.n - m_allowed.n) + 1.0f;
            if (std::abs(m_factor - old) > 0.05f) {
                mark_dirty();
            }
        }
        m_n++;
        if (m_n >= s_history_length) {
            m_n = 0;
            m_mean_history[m_current_mean_index] = m_current.m;
            m_current_mean_index++;
            if (m_current_mean_index >= s_history_length) {
                m_mean_full = true;
                m_current_mean_index = 0;
            }
            if (m_mean_full) {
                float av = mean_average();
                if (((m_default.m - av) / m_default.s) < m_default.n) {
                    m_allowed.m = av;
                } else {
                    m_allowed .m = (m_default.m - m_default.s*m_default.n);
                }
            }
        }
        m_current_count = 0;
        m_last = std::chrono::system_clock::now();
    }
}

auto RateSupervisor::current() const -> RateSupervisor::Rate
{
    return m_current;
}

auto RateSupervisor::factor() const -> float
{
    return m_factor;
}

auto RateSupervisor::dirty() -> bool
{
    if (m_dirty) {
        m_dirty = false;
        return true;
    }
    return false;
}

auto RateSupervisor::average() -> float
{
    float total { 0 };
    for (std::size_t i { 0 }; i < s_history_length; i++) {
        total += m_history[i];
    }
    return total / static_cast<float>(s_history_length);
}

auto RateSupervisor::mean_average() -> float
{
    float total { 0 };
    for (std::size_t i { 0 }; i < s_history_length; i++) {
        total += m_mean_history[i];
    }
    return total / static_cast<float>(s_history_length);
}


void RateSupervisor::mark_dirty()
{
    m_dirty = true;
}

}
