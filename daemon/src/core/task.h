#ifndef TASK_H
#define TASK_H

#include <chrono>
#include <functional>

using used_clock = std::chrono::steady_clock;
using time_point = used_clock::time_point;


struct Task {
    std::function<void()> func;
    time_point time;
    std::chrono::milliseconds interval{0}; // 0 for one-shot

    bool operator<(const Task& other) const {
        return time > other.time; // min-heap
    }
};

#endif // TASK_H