#include "utility/ublox_ratebuffer.h"

#include "data/muondetector_structs.h"

CounterRateBuffer::CounterRateBuffer(unsigned int counter_mask)
    : m_counter_mask(counter_mask), m_instance_start(TimestampClockType::now()) {
}

void CounterRateBuffer::onCounterValue(uint16_t value) {
    EventTime event_time{TimestampClockType::now()};
    m_countbuffer.emplace_back(event_time, value);
    if (m_countbuffer.size() == 1) {
        return;
    }

    while (m_countbuffer.size() > 2 && (event_time - m_countbuffer.front().first > m_buffer_time)) {
        m_countbuffer.pop_front();
    }
}

auto CounterRateBuffer::avgRate() const -> double {
    if (m_countbuffer.size() < 2) {
        return 0.;
    }
    auto tend = TimestampClockType::now();
    auto tstart = tend - m_buffer_time;
    if (tstart < m_instance_start) {
        tstart = m_instance_start;
    }
    unsigned int total_counts{0};

    auto buffer_iter{m_countbuffer.begin()};
    while (buffer_iter != std::prev(m_countbuffer.end())) {
        if (buffer_iter->first < tstart) {
            tstart = std::next(buffer_iter)->first;
            ++buffer_iter;
            continue;
        }
        int diff_count = std::next(buffer_iter)->second - buffer_iter->second;
        if (diff_count < 0) {
            diff_count += m_counter_mask + 1;
        }
        total_counts += diff_count;
        ++buffer_iter;
    }

    double span =
        1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count();
    return (total_counts / span);
}
