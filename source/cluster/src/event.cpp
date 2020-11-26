#include "event.h"

namespace MuonPi {

Event::Event(std::size_t hash, std::uint64_t id, std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end) noexcept
    : m_start { start }
    , m_end { end }
    , m_hash { hash }
    , m_id { id}
{}

Event::Event(std::size_t hash, std::uint64_t id, std::chrono::steady_clock::time_point start, std::chrono::steady_clock::duration duration) noexcept
    : m_start { start }
    , m_end { start + duration }
    , m_hash { hash }
    , m_id { id}
{}

Event::~Event() noexcept = default;

auto Event::start() const noexcept -> std::chrono::steady_clock::time_point
{
    return m_start;
}

auto Event::duration() const noexcept -> std::chrono::steady_clock::duration
{
    return m_end - m_start;
}

auto Event::end() const noexcept -> std::chrono::steady_clock::time_point
{
    return m_end;
}

auto Event::id() const noexcept -> std::uint64_t
{
    return m_id;
}

auto Event::hash() const noexcept -> std::size_t
{
    return m_hash;
}

auto Event::n() const noexcept -> std::size_t
{
    return m_n;
}


}
