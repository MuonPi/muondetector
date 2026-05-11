

#include "scheduler.h"

#include "task.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <mutex>
#include <pthread.h>

Scheduler::Scheduler(ThreadPool& pool) : threadPool(pool) {
}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    running = true;
    thread = std::thread(&Scheduler::loop, this);
}

void Scheduler::stop() {
    {
        std::lock_guard lock(mutex);

        running = false;

        decltype(queue) empty;
        queue.swap(empty);
    }

    cv.notify_all();

    if (thread.joinable()) {
        thread.join();
    }
}

void Scheduler::loop() {

    pthread_setname_np(pthread_self(), "muondet-scheduler");
    std::unique_lock lock(mutex);

    while (running) {
        if (queue.empty()) {
            cv.wait(lock);
            continue;
        }

        auto next = queue.top();
        auto now = used_clock::now();

        if (next.time > now) {
            cv.wait_until(lock, next.time);
            continue;
        }

        queue.pop();

        // dispatch to thread pool
        threadPool.enqueue(next.func);

        // reschedule if periodic
        if (running.load() && next.interval.count() > 0) {
            next.time += next.interval;
            queue.push(next);
        }
    }
    while (!queue.empty()) {
        queue.pop();
    }
}

void Scheduler::every(std::chrono::milliseconds interval, std::function<void()> func) {
    schedule(func, used_clock::now(), interval);
}

void Scheduler::once(std::function<void()> func, time_point time) {
    schedule(func, time);
}

void Scheduler::once(std::function<void()> func, std::size_t milliseconds) {
    auto now = used_clock::now();
    auto time =
        now + static_cast<used_clock::duration>(milliseconds * static_cast<std::size_t>(1e6));
    schedule(func, time);
}

void Scheduler::schedule(std::function<void()> func, time_point time,
                         std::chrono::milliseconds interval) {
    {
        std::lock_guard lock(mutex);
        queue.push({func, time, interval});
    }
    cv.notify_one();
}