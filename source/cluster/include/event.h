#ifndef EVENT_H
#define EVENT_H

#include <chrono>
#include <vector>

namespace MuonPi {

/**
 * @brief Event ID generator function
 *  function to generate a unique event id from the system time and a random number
 */
std::size_t generate_unique_event_id();

/**
 * @brief The Event class
 *  class, which is used as an interface for single events and combined events
 */
class Event
{
public:
    /**
     * @brief Event
     * @param hash a hash of the username and detector site_id
     * @param id The id of the event
     * @param start The start time of the event
     * @param end The end time of the event
     */
    Event(std::size_t hash, std::uint64_t id, std::chrono::system_clock::time_point start = {}, std::chrono::system_clock::time_point end = {}) noexcept;

    /**
     * @brief Event
     * @param hash a hash of the username and detector site_id
     * @param id The id of the event
     * @param start The start time of the event
     * @param duration The duration of the event
     */
    Event(std::size_t hash, std::uint64_t id, std::chrono::system_clock::time_point start = {}, std::chrono::system_clock::duration duration = {}) noexcept;

    Event(std::uint64_t id, Event event) noexcept;

    Event() noexcept;

    Event(const Event& other);
    Event(Event&& other);

    virtual ~Event() noexcept;

    /**
     * @brief start
     * @return The starting time of the event
     */
    [[nodiscard]] auto start() const noexcept -> std::chrono::system_clock::time_point;

    /**
     * @brief duration
     * @return The duration of the event
     */
    [[nodiscard]] auto duration() const noexcept -> std::chrono::system_clock::duration;

    /**
     * @brief end
     * @return The end time of the event
     */
    [[nodiscard]] auto end() const noexcept -> std::chrono::system_clock::time_point;

    /**
     * @brief id
     * @return The id of this event
     */
    [[nodiscard]] auto id() const noexcept -> std::uint64_t;

    /**
     * @brief hash
     * @return The hash of the detector for this event
     */
    [[nodiscard]] auto hash() const noexcept -> std::size_t;

    /**
     * @brief n
     * @return The number of events of this event. 1 for a default event
     */
    [[nodiscard]] auto n() const noexcept -> std::size_t;

    /**
     * @brief contested
     * @return Return whether this event is contested or not.
     * @see mark_contested
     */
    [[nodiscard]] auto contested() const -> bool;

    /**
      * @brief Get the list of events. Moves the vector.
      * @return The list of events contained in this combined event
      */
    [[nodiscard]] auto events() -> std::vector<Event>;


    /**
     * @brief add_event Adds an event to the CombinedEvent.
     * @param event The event to add. In the case that the abstract event is a combined event, the child events will be added instead of their combination.
     * 				The unique_ptr will be moved.
     */
    void add_event(Event event) noexcept;

    /**
     * @brief mark_contested Mark this combined event as containing a set of events which have at least one possible subset of events though does not work as an event in its totality.
     */
    void mark_contested();

    [[nodiscard]] auto valid() const -> bool;

private:
    std::chrono::system_clock::time_point m_start {};
    std::chrono::system_clock::time_point m_end {};
    std::size_t m_n { 1 };
    std::vector<Event> m_events {};
    bool m_contested { false };
    std::uint64_t m_hash {};
    std::uint64_t m_id {};
    bool m_valid { true };
};
}

#endif // EVENT_H
