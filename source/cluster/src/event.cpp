#include "event.h"
#include "log.h"

#include <random>

namespace MuonPi {

Event::Event(std::size_t hash, std::uint64_t id, Data data) noexcept
    : m_hash { hash }
    , m_id { id }
    , m_data { std::move(data) }
{
}

Event::Event(std::uint64_t id, Event event) noexcept
    : Event{event.hash(), id, event.data()}
{
    m_data.end = m_data.start;

    m_events.push_back(std::move(event));
}

Event::Event() noexcept
    : m_valid { false }
{
}

Event::Event(const Event& other)
    : m_n { other.m_n }
    , m_events { other.m_events }
    , m_hash { other.m_hash }
    , m_id { other.m_id }
    , m_valid { other.m_valid }
    , m_data { other.m_data }
    , m_detector { other.m_detector }
{
}

Event::Event(Event&& other)
    : m_n { std::move(other.m_n) }
    , m_events { std::move(other.m_events) }
    , m_hash { std::move(other.m_hash) }
    , m_id { std::move(other.m_id) }
    , m_valid { std::move(other.m_valid) }
    , m_data { std::move(other.m_data) }
    , m_detector { std::move(other.m_detector) }
{
}
auto Event::operator=(const Event& other) -> Event&
{
    m_n = other.m_n;
    m_events = other.m_events;
    m_hash = other.m_hash;
    m_id = other.m_id;
    m_valid = other.m_valid;
    m_data = other.m_data;
    m_detector = other.m_detector;

    return *this;
}

auto Event::operator=(Event&& other) -> Event&
{
    m_n = std::move(other.m_n);
    m_events = std::move(other.m_events);
    m_hash = std::move(other.m_hash);
    m_id = std::move(other.m_id);
    m_valid = std::move(other.m_valid);
    m_data = std::move(other.m_data);
    m_detector = std::move(other.m_detector);

    return *this;
}

Event::~Event() noexcept = default;


auto Event::epoch() const noexcept -> std::int_fast64_t
{
    return m_data.epoch;
}

auto Event::start() const noexcept -> std::int_fast64_t
{
    return m_data.start;
}

auto Event::duration() const noexcept -> std::int_fast64_t
{
    return m_data.end;
}

auto Event::end() const noexcept -> std::int_fast64_t
{
    return m_data.end;
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
    if (event.n() > 1) {
        for (auto e: event.events()) {
            add_event(e);
        }
        return;
    }

    std::int_fast64_t offset { (epoch() - event.epoch()) * static_cast<std::int_fast64_t>(1e9) + (start() - event.start()) };


    if (offset > 0) {
        m_data.start -= offset;
    } else if ((start() - offset) > end()) {
        m_data.end = start() - offset;
    }


    m_events.push_back(std::move(event));
    m_n++;
}

auto Event::events() -> const std::vector<Event>&
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

void Event::set_detector(std::shared_ptr<Detector> detector)
{
    m_detector = detector;
}


}
