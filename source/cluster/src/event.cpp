#include "event.h"
#include "log.h"

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

Event::Event(std::size_t hash, std::uint64_t id, Data data) noexcept
    : m_start { data.start }
    , m_end { data.end }
    , m_hash { hash }
    , m_id { id }
    , m_data { std::move(data) }
{
}

Event::Event(std::size_t hash, std::uint64_t id, std::chrono::system_clock::time_point start, std::chrono::system_clock::duration duration) noexcept
    : m_start { start }
    , m_end { start + duration }
    , m_hash { hash }
    , m_id { id}
{}

Event::Event(std::uint64_t id, Event event) noexcept
    : Event{event.hash(), id, event.start(), event.start()}
{
    m_events.push_back(std::move(event));
}

Event::Event() noexcept
    : m_valid { false }
{
}

Event::Event(const Event& other)
    : m_start { other.m_start }
    , m_end { other.m_end }
    , m_n { other.m_n }
    , m_events { other.m_events }
    , m_contested { other.m_contested }
    , m_hash { other.m_hash }
    , m_id { other.m_id }
    , m_valid { other.m_valid }
    , m_data { other.m_data }
{
}

Event::Event(Event&& other)
    : m_start { std::move(other.m_start) }
    , m_end { std::move(other.m_end) }
    , m_n { std::move(other.m_n) }
    , m_events { std::move(other.m_events) }
    , m_contested { std::move(other.m_contested) }
    , m_hash { std::move(other.m_hash) }
    , m_id { std::move(other.m_id) }
    , m_valid { std::move(other.m_valid) }
    , m_data { std::move(other.m_data) }
{
}

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

void Event::add_event(Event event) noexcept
{
    if (event.start() <= start()) {
        m_start = event.start();
    }
    if (event.start() >= end()) {
        m_end = event.start();
    }
    m_events.push_back(std::move(event));
    m_n++;
}

void Event::mark_contested()
{
    m_contested = true;
}

auto Event::contested() const -> bool
{
    return m_contested;
}

auto Event::events() -> std::vector<Event>
{
    return m_events;
}

auto Event::valid() const -> bool
{
    return m_valid;
}
auto Event::data() const -> Data
{
    return m_data;
}

void Event::set_data(const Data& data)
{
    m_data = data;
}


}
