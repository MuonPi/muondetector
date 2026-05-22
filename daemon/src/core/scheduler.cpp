

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

void Scheduler::cancel(std::size_t id) {
    std::lock_guard lock(mutex);
    cancelled.insert(id);
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

        if (cancelled.contains(next.id)) {
            continue;
        }

        auto task = std::move(next);
        lock.unlock();

        bool keep_running = true;

        if (running.load()) {
            // call task, if returns true, keep running
            keep_running = task.func();
        }

        if (running.load() && keep_running && task.interval.count() > 0 &&
            !cancelled.contains(task.id)) {
            // only re-schedule if it has some interval value (is repeating task)
            // and only re-schedule if function did return true
            // and only if "cancelled" does not contain task id
            task.time = used_clock::now() + task.interval;
            lock.lock();
            queue.push(std::move(task));
            lock.unlock();
        }
        lock.lock();
    }
    while (!queue.empty()) {
        queue.pop();
    }
}

auto Scheduler::every(std::chrono::milliseconds interval,
                      std::function<bool()> func) -> std::size_t {
    return schedule(func, used_clock::now(), interval);
}

auto Scheduler::once(std::function<bool()> func, time_point time) -> std::size_t {
    return schedule(func, time);
}

auto Scheduler::once(std::function<bool()> func, std::size_t milliseconds) -> std::size_t {
    auto now = used_clock::now();
    auto time =
        now + static_cast<used_clock::duration>(milliseconds * static_cast<std::size_t>(1e6));
    return schedule(func, time);
}

auto Scheduler::schedule(std::function<bool()> func, time_point time,
                         std::chrono::milliseconds interval) -> std::size_t {
    std::size_t id = next_id++;
    Task task{.func = std::move(func), .time = time, .interval = interval, .id = id};
    {

        std::lock_guard lock(mutex);
        queue.push(std::move(task));
    }
    cv.notify_one();
    return id;
}