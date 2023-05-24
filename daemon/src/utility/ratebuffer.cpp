#include "utility/ratebuffer.h"
#include <iostream>

constexpr auto invalid_time = std::chrono::system_clock::time_point::min();

/*
<<<<<<< HEAD
RateBuffer::RateBuffer(QObject* parent)
    : QObject(parent)
{
}

void RateBuffer::updateAllIntervals(unsigned int new_gpio, EventTime new_event_time)
{
    for (const auto [other_gpio, buffer_item] : buffermap) {
        if (new_gpio == other_gpio)
            continue;
        if (buffer_item.eventbuffer.empty())
            continue;
        auto last_other_time = buffer_item.eventbuffer.back();
        std::chrono::nanoseconds nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(new_event_time - last_other_time);
        fIntervalMap.insert_or_assign(std::make_pair(new_gpio, other_gpio), nsecs);
    }
}

void RateBuffer::onEvent(unsigned int gpio, EventTime event_time)
{
    if (buffermap[gpio].eventbuffer.empty()) {
        buffermap[gpio].eventbuffer.push(event_time);
        emit filteredEvent(gpio, event_time);
        return;
    }
    auto last_event_time = buffermap[gpio].eventbuffer.back();
    if (event_time - last_event_time < buffermap[gpio].current_deadtime) {
        //			buffermap[gpio].eventbuffer.push(event_time);
        return;
    }
    updateAllIntervals(gpio, event_time);

    while (!buffermap.at(gpio).eventbuffer.empty()
        && (event_time - buffermap[gpio].eventbuffer.front() > fBufferTime)) {
        buffermap[gpio].eventbuffer.pop();
    }

    if (!buffermap[gpio].eventbuffer.empty()) {
        buffermap[gpio].last_interval = std::chrono::duration_cast<std::chrono::nanoseconds>(event_time - last_event_time);
        if (event_time - last_event_time < MAX_DEADTIME) {
            //			std::cout << "now-last:"<<(now-last_event_time)/1us<<" dt="<<buffermap[gpio].current_deadtime.count()<<std::endl;
            if (buffermap[gpio].current_deadtime < MAX_DEADTIME) {
                buffermap[gpio].current_deadtime += DEADTIME_INCREMENT;
                //				std::cout << std::dec << "adjusting deadtime for gpio " << gpio << " to " << buffermap[gpio].current_deadtime/1us << "us" << std::endl;
            }
            if (event_time - last_event_time < buffermap[gpio].current_deadtime) {
                buffermap[gpio].eventbuffer.push(event_time);
                return;
            }
        } else {
            auto deadtime = buffermap[gpio].current_deadtime;
            if (deadtime > DEADTIME_INCREMENT) {
                buffermap[gpio].current_deadtime -= DEADTIME_INCREMENT;
            } else if (deadtime > 0s) {
                buffermap[gpio].current_deadtime -= std::chrono::microseconds(1);
            }
        }
    }
    buffermap[gpio].eventbuffer.push(event_time);
    emit filteredEvent(gpio, event_time);
    if (buffermap[gpio].last_interval != std::chrono::nanoseconds(0)) {
        emit eventIntervalSignal(gpio, buffermap[gpio].last_interval);
    }
}

auto RateBuffer::avgRate(unsigned int gpio) const -> double
{
    auto it = buffermap.find(gpio);
    if (it == buffermap.end())
        return 0.;
    if (it->second.eventbuffer.empty())
        return 0.;
    auto end = std::chrono::system_clock::now();
    auto start = end - fBufferTime;
    if (start > it->second.eventbuffer.front())
        start = it->second.eventbuffer.front();
    double span = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    return (it->second.eventbuffer.size() / span);
}

auto RateBuffer::currentDeadtime(unsigned int gpio) const -> std::chrono::microseconds
{
    auto it = buffermap.find(gpio);
    if (it == buffermap.end())
        return std::chrono::microseconds(0);
    return it->second.current_deadtime;
}

auto RateBuffer::lastInterval(unsigned int gpio) const -> std::chrono::nanoseconds
{
    auto it = buffermap.find(gpio);
    if (it == buffermap.end() || it->second.eventbuffer.size() < 2)
        return std::chrono::nanoseconds(0);
    return it->second.last_interval;
}

auto RateBuffer::lastInterval(unsigned int gpio1, unsigned int gpio2) const -> std::chrono::nanoseconds
{
    auto it = fIntervalMap.find(std::make_pair(gpio1, gpio2));
    if (it == fIntervalMap.end())
        return std::chrono::nanoseconds(0);
    return fIntervalMap.at(std::make_pair(gpio1, gpio2));
}

auto RateBuffer::lastEventTime(unsigned int gpio) const -> EventTime
{
    auto it = buffermap.find(gpio);
    if (it == buffermap.end() || it->second.eventbuffer.empty())
        return invalid_time;
    return it->second.eventbuffer.back();
=======
*/
CounterRateBuffer::CounterRateBuffer(unsigned int counter_mask, QObject* parent)
    : QObject(parent)
    , m_counter_mask(counter_mask)
    , m_instance_start(std::chrono::system_clock::now())
{
}

void CounterRateBuffer::onCounterValue(uint16_t value)
{
    EventTime event_time { std::chrono::system_clock::now() };
    m_countbuffer.emplace_back(event_time, value);
    if (m_countbuffer.size() == 1) {
        return;
    }

    while (m_countbuffer.size() > 2
        && (event_time - m_countbuffer.front().first > m_buffer_time)) {
        m_countbuffer.pop_front();
    }
}

auto CounterRateBuffer::avgRate() const -> double
{
    if (m_countbuffer.size() < 2) {
        return 0.;
    }
    auto tend = std::chrono::system_clock::now();
    auto tstart = tend - m_buffer_time;
    if (tstart < m_instance_start) {
        tstart = m_instance_start;
    }
    unsigned int total_counts { 0 };

    auto buffer_iter { m_countbuffer.begin() };
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

    double span = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count();
    return (total_counts / span);
}

EventRateBuffer::EventRateBuffer(unsigned int gpio, QObject* parent)
    : QObject(parent)
    , m_gpio(gpio)
    , m_instance_start(std::chrono::system_clock::now())
{
}

void EventRateBuffer::clear()
{
    m_eventbuffer = std::queue<EventTime, std::list<EventTime>> {};
    m_instance_start = std::chrono::system_clock::now();
}

void EventRateBuffer::onEvent(uint8_t gpio)
{
    if (gpio != m_gpio)
        return;
    EventTime event_time { std::chrono::system_clock::now() };
    if (m_eventbuffer.empty()) {
        m_eventbuffer.push(event_time);
        emit filteredEvent(gpio, event_time);
        return;
    }

    auto last_event_time = m_eventbuffer.back();
    if (event_time - last_event_time < m_current_deadtime) {
        // m_buffer[gpio].eventbuffer.push(event_time);
        return;
    }

    while (!m_eventbuffer.empty()
        && (event_time - m_eventbuffer.front() > m_buffer_time)) {
        m_eventbuffer.pop();
    }

    if (!m_eventbuffer.empty()) {
        m_last_interval = std::chrono::duration_cast<std::chrono::nanoseconds>(event_time - last_event_time);
        if (event_time - last_event_time < MAX_DEADTIME) {
            //			std::cout << "now-last:"<<(now-last_event_time)/1us<<" dt="<<buffermap[gpio].current_deadtime.count()<<std::endl;
            if (m_current_deadtime < MAX_DEADTIME) {
                m_current_deadtime += DEADTIME_INCREMENT;
                // std::cout << std::dec << "adjusting deadtime for gpio " << gpio << " to " << m_buffer.current_deadtime/1us << "us" << std::endl;
            }
            if (event_time - last_event_time < m_current_deadtime) {
                m_eventbuffer.push(event_time);
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
    m_eventbuffer.push(event_time);
    emit filteredEvent(gpio, event_time);
    if (m_last_interval != std::chrono::nanoseconds(0)) {
        emit eventIntervalSignal(gpio, m_last_interval);
    }
}

auto EventRateBuffer::avgRate() const -> double
{
    if (m_eventbuffer.empty())
        return 0.;
    auto tend = std::chrono::system_clock::now();
    auto tstart = tend - m_buffer_time;
    if (tstart < m_instance_start)
        tstart = m_instance_start;
    if (tstart > m_eventbuffer.front())
        tstart = m_eventbuffer.front();
    double span = 1e-6 * std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count();
    return (m_eventbuffer.size() / span);
}

auto EventRateBuffer::currentDeadtime() const -> std::chrono::microseconds
{
    return m_current_deadtime;
}

auto EventRateBuffer::lastInterval() const -> std::chrono::nanoseconds
{
    if (m_eventbuffer.size() < 2)
        return std::chrono::nanoseconds(0);
    return m_last_interval;
}

auto EventRateBuffer::lastEventTime() const -> EventTime
{
    if (m_eventbuffer.empty())
        return invalid_time;
    return m_eventbuffer.back();
//>>>>>>> dev
}
