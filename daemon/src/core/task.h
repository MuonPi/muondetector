#ifndef TASK_H
#define TASK_H

#include <chrono>
#include <functional>

using used_clock = std::chrono::steady_clock;
using time_point = used_clock::time_point;

struct Task {
    std::function<bool()> func; // if returns false: cancel future schedules
    time_point time;
    std::chrono::milliseconds interval{0};
    std::size_t id{0};

    bool operator<(const Task& other) const { return time > other.time; }
};

#endif // TASK_H