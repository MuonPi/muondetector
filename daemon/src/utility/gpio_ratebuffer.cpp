#include "utility/gpio_ratebuffer.h"

#include "core/event_bus.h"
#include "data/events/gpio_event.h"
#include "data/events/interval_event.h"
#include "gpio_pin_definitions.h"

#include <iostream>

constexpr auto invalid_time = TimestampClockType::time_point::min();

// CounterRateBuffer::CounterRateBuffer(unsigned int counter_mask)
//     : m_counter_mask(counter_mask)
//     , m_instance_start(TimestampClockType::now())
// {
// }

// void CounterRateBuffer::onCounterValue(uint16_t value)
// {
//     EventTime event_time { TimestampClockType::now() };
//     m_countbuffer.emplace_back(event_time, value);
//     if (m_countbuffer.size() == 1) {
//         return;
//     }

//     while (m_countbuffer.size() > 2
//         && (event_time - m_countbuffer.front().first > m_buffer_time)) {
//         m_countbuffer.pop_front();
//     }
// }

// auto CounterRateBuffer::avgRate() const -> double
// {
//     if (m_countbuffer.size() < 2) {
//         return 0.;
//     }
//     auto tend = TimestampClockType::now();
//     auto tstart = tend - m_buffer_time;
//     if (tstart < m_instance_start) {
//         tstart = m_instance_start;
//     }
//     unsigned int total_counts { 0 };

//     auto buffer_iter { m_countbuffer.begin() };
//     while (buffer_iter != std::prev(m_countbuffer.end())) {
//         if (buffer_iter->first < tstart) {
//             tstart = std::next(buffer_iter)->first;
//             ++buffer_iter;
//             continue;
//         }
//         int diff_count = std::next(buffer_iter)->second - buffer_iter->second;
//         if (diff_count < 0) {
//             diff_count += m_counter_mask + 1;
//         }
//         total_counts += diff_count;
//         ++buffer_iter;
//     }

//     double span = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(tend -
//     tstart).count(); return (total_counts / span);
// }

EventRateBuffer::EventRateBuffer(EventBus& bus, std::optional<EventEdge> filterEdge)
    : bus_{bus}, m_filterEdge(filterEdge), m_instance_start(TimestampClockType::now()) {
}

void EventRateBuffer::clear() {
    m_eventbuffer = std::deque<EventTime>{};
    m_instance_start = TimestampClockType::now();
}

void EventRateBuffer::handle(const GpioEvent& event) {
    if (m_filterEdge.has_value() && (event.edge != m_filterEdge.value())) {
        return;
    }

    if (m_eventbuffer.empty()) {
        m_eventbuffer.push_back(event.timestamp);
        bus_.publish(event);
        return;
    }

    auto last_event_time = m_eventbuffer.back();
    auto diff = event.timestamp - last_event_time;
    if (diff < m_current_deadtime) {
        bus_.publish(IntervalEvent{.sig = event.gpio_signal, .interval = diff});
        return;
    }

    while (!m_eventbuffer.empty() && (event.timestamp - m_eventbuffer.front() > m_buffer_time)) {
        m_eventbuffer.pop_front();
    }

    if (!m_eventbuffer.empty()) {
        m_last_interval =
            std::chrono::duration_cast<std::chrono::nanoseconds>(event.timestamp - last_event_time);
        if (m_last_interval < MAX_DEADTIME) {
            //			std::cout << "now-last:"<<(now-last_event_time)/1us<<"
            // dt="<<buffermap[sig].current_deadtime.count()<<std::endl;
            if (m_current_deadtime < MAX_DEADTIME) {
                m_current_deadtime += DEADTIME_INCREMENT;
                // std::cout << std::dec << "adjusting deadtime for sig " << sig << " to " <<
                // m_buffer.current_deadtime/1us << "us" << std::endl;
            }
            if (event.timestamp - last_event_time < m_current_deadtime) {
                m_eventbuffer.push_back(event.timestamp);
                return;
            }
        } else {
            auto deadtime = m_current_deadtime;
            if (deadtime > DEADTIME_INCREMENT) {
                m_current_deadtime -= DEADTIME_INCREMENT;
            } else if (deadtime > 0s) {
                m_current_deadtime -= std::chrono::microseconds(1);
            }
        }
    }
    m_eventbuffer.push_back(event.timestamp);
    bus_.publish(event);
    if (m_last_interval != std::chrono::nanoseconds(0)) {
        bus_.publish(IntervalEvent{.sig = event.gpio_signal, .interval = m_last_interval});
    }
}

auto EventRateBuffer::avgRate() const -> double {
    if (m_eventbuffer.empty())
        return 0.;
    auto tend = TimestampClockType::now();
    auto tstart = tend - m_buffer_time;
    if (tstart < m_instance_start)
        tstart = m_instance_start;
    if (tstart > m_eventbuffer.front())
        tstart = m_eventbuffer.front();
    double span =
        1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count();
    return (m_eventbuffer.size() / span);
}

auto EventRateBuffer::currentDeadtime() const -> std::chrono::microseconds {
    return m_current_deadtime;
}

auto EventRateBuffer::lastInterval() const -> std::chrono::nanoseconds {
    if (m_eventbuffer.size() < 2)
        return std::chrono::nanoseconds(0);
    return m_last_interval;
}

auto EventRateBuffer::lastEventTime() const -> EventTime {
    if (m_eventbuffer.empty())
        return invalid_time;
    return m_eventbuffer.back();
}

// CoincidenceEventBuffer::CoincidenceEventBuffer(unsigned int event_sig, EventEdge filterEdge,
// unsigned int coinc_sig, bool anti_coinc, QObject* parent)
//     : EventRateBuffer(event_sig, filterEdge, parent)
//     , m_coinc_sig(coinc_sig)
//     , m_is_veto(anti_coinc)
// {
// }

// auto CoincidenceEventBuffer::onEvent(const GpioEvent& event) -> bool
// {
//     if (m_filterEdge.hasValue() && (edge == m_filterEdge.value())) {
//         return false;
//     }
//     if (sig == m_coinc_sig) {
// 		m_last_coinc_event = event_time;
// 		long int distance_after_us =
// std::chrono::duration_cast<std::chrono::microseconds>(event_time - lastEventTime()).count();
// if (m_is_veto) { 			if ( std::abs(distance_after_us) <= COINCIDENCE_WINDOW.count()) {
// 				//event is inside veto window
// 				m_eventbuffer.pop_back();
// 			}
// 		} else {
// 			long int distance_before_us =
// std::chrono::duration_cast<std::chrono::microseconds>(m_last_coinc_event -
// lastEventTime()).count(); 			if ( std::abs(distance_after_us) >
// COINCIDENCE_WINDOW.count() && std::abs(distance_before_us) > COINCIDENCE_WINDOW.count()) {
// 				//event is outside coinc window
// 				m_eventbuffer.pop_back();
// 			}
// 		}
// 		return;
// 	}

//     if (m_eventbuffer.empty()) {
// 		long int distance_us = std::chrono::duration_cast<std::chrono::microseconds>(event_time -
// m_last_coinc_event).count(); 		if (!m_is_veto) { 			if ( std::abs(distance_us) <=
// COINCIDENCE_WINDOW.count()) {
// 				//event is inside coinc window
// 				m_eventbuffer.push_back(event_time);
// 			}
// 		} else {
// 			if ( std::abs(distance_us) > COINCIDENCE_WINDOW.count()) {
// 				//event is outside veto window
// 				m_eventbuffer.push_back(event_time);
// 			}
// 		}
// 		return;
//     }

//     auto last_event_time = m_eventbuffer.back();
//     if (event_time - last_event_time < m_current_deadtime) {
//         // m_buffer[sig].eventbuffer.push(event_time);
//         return;
//     }

//     while (!m_eventbuffer.empty()
//         && (event_time - m_eventbuffer.front() > m_buffer_time)) {
//         m_eventbuffer.pop_front();
//     }

//     if (!m_eventbuffer.empty()) {
//         m_last_interval = std::chrono::duration_cast<std::chrono::nanoseconds>(event_time -
//         last_event_time); if (event_time - last_event_time < MAX_DEADTIME) {
//             //			std::cout << "now-last:"<<(now-last_event_time)/1us<<"
//             dt="<<buffermap[sig].current_deadtime.count()<<std::endl; if (m_current_deadtime <
//             MAX_DEADTIME) {
//                 m_current_deadtime += DEADTIME_INCREMENT;
//                 // std::cout << std::dec << "adjusting deadtime for sig " << sig << " to " <<
//                 m_buffer.current_deadtime/1us << "us" << std::endl;
//             }
//             if (event_time - last_event_time < m_current_deadtime) {
// 				long int distance_us =
// std::chrono::duration_cast<std::chrono::microseconds>(event_time - m_last_coinc_event).count();
// 				if (!m_is_veto) {
// 					if ( std::abs(distance_us) <= COINCIDENCE_WINDOW.count()) {
// 						//event is inside coinc window
// 						m_eventbuffer.push_back(event_time);
// 					}
// 				} else {
// 					if ( std::abs(distance_us) > COINCIDENCE_WINDOW.count()) {
// 						//event is outside veto window
// 						m_eventbuffer.push_back(event_time);
// 					}
// 				}
//                 return;
//             }
//         } else {
//             auto deadtime = m_current_deadtime;
//             if (deadtime > DEADTIME_INCREMENT) {
//                 m_current_deadtime -= DEADTIME_INCREMENT;
//             } else if (deadtime > 0s) {
//                 m_current_deadtime -= std::chrono::microseconds(1);
//             }
//         }
//     }
//     m_eventbuffer.push_back(event_time);
// }
