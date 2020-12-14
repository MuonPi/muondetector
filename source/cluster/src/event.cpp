#include "event.h"
#include <random>

namespace MuonPi {

std::size_t generate_unique_event_id() {

    //std::size_t a;
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<std::size_t> distrib;
    return distrib(gen);
}


Event::Event(std::size_t hash, std::uint64_t id, std::chrono::system_clock::time_point start, std::chrono::system_clock::time_point end) noexcept
    : m_start { start }
    , m_end { end }
    , m_hash { hash }
    , m_id { id}
{}

Event::Event(std::size_t hash, std::uint64_t id, std::chrono::system_clock::time_point start, std::chrono::system_clock::duration duration) noexcept
    : m_start { start }
    , m_end { start + duration }
    , m_hash { hash }
    , m_id { id}
{}

Event::~Event() noexcept = default;

auto Event::start() const noexcept -> std::chrono::system_clock::time_point
{
    return m_start;
}

auto Event::duration() const noexcept -> std::chrono::system_clock::duration
{
    return m_end - m_start;
}

auto Event::end() const noexcept -> std::chrono::system_clock::time_point
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
