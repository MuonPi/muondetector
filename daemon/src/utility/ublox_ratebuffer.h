#ifndef UBLOX_RATEBUFFER_H
#define UBLOX_RATEBUFFER_H

#include "data/muondetector_structs.h"

#include <chrono>
#include <limits>
#include <list>

using namespace std::literals;

constexpr std::chrono::microseconds MAX_BUFFER_TIME{30s};

class CounterRateBuffer {

  public:
    CounterRateBuffer(unsigned int counter_mask =
                          static_cast<unsigned int>(std::numeric_limits<std::uint16_t>::max()));
    ~CounterRateBuffer() = default;
    void clear();

    [[nodiscard]] auto avgRate() const -> double;
    [[nodiscard]] auto lastEventTime() const -> EventTime;
    void setBufferTime(std::chrono::seconds buftime) {
        m_buffer_time = std::chrono::duration_cast<std::chrono::microseconds>(buftime);
    }

    void onCounterValue(uint16_t value);

  private:
    unsigned int m_counter_mask{};
    std::chrono::microseconds m_buffer_time{MAX_BUFFER_TIME};
    std::list<std::pair<EventTime, std::uint16_t>> m_countbuffer{};
    std::chrono::nanoseconds m_last_interval{0};
    EventTime m_instance_start{};
};

#endif // UBLOX_RATEBUFFER_H