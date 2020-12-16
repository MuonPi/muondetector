#ifndef EVENT_H
#define EVENT_H

#include <chrono>
#include <vector>
#include <string>
#include <memory>

namespace MuonPi {

class Detector;

/**
 * @brief The Event class
 *  class, which is used as an interface for single events and combined events
 */
class Event
{
public:
    struct Data {
        std::string user {};
        std::string station_id {};
        std::int_fast64_t epoch {};
        std::int_fast32_t start {};
        std::int_fast32_t end {};
        std::uint32_t time_acc {};
        std::uint16_t ublox_counter {};
        std::uint8_t fix {};
        std::uint8_t utc {};
        std::uint8_t gnss_time_grid {};
    };

    Event(std::size_t hash, std::uint64_t id, Data data) noexcept;

    Event(std::uint64_t id, Event event) noexcept;

    Event() noexcept;

    Event(const Event& other);
    Event(Event&& other);

    virtual ~Event() noexcept;


    void set_detector(std::shared_ptr<Detector> detector);

    /**
     * @brief end
     * @return The end time of the event
     */
    [[nodiscard]] auto epoch() const noexcept -> std::int_fast64_t;

    /**
     * @brief start
     * @return The starting time of the event
     */
    [[nodiscard]] auto start() const noexcept -> std::int_fast64_t;

    /**
     * @brief duration
     * @return The duration of the event
     */
    [[nodiscard]] auto duration() const noexcept -> std::int_fast64_t;

    /**
     * @brief end
     * @return The end time of the event
     */
    [[nodiscard]] auto end() const noexcept -> std::int_fast64_t;

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

    [[nodiscard]] auto valid() const -> bool;

    [[nodiscard]] auto data() const -> Data;

    void set_data(const Data& data);

private:
    std::size_t m_n { 1 };
    std::vector<Event> m_events {};
    std::uint64_t m_hash {};
    std::uint64_t m_id {};
    bool m_valid { true };

    Data m_data {};

    std::shared_ptr<Detector> m_detector { nullptr };
};
}

#endif // EVENT_H
