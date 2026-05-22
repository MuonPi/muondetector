#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "task.h"
#include "thread_pool.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>

class Scheduler {
  public:
    explicit Scheduler(ThreadPool& pool);
    ~Scheduler();

    void start();
    void stop();
    void cancel(std::size_t id);

    auto every(std::chrono::milliseconds interval, std::function<bool()> func) -> std::size_t;
    auto once(std::function<bool()> func, time_point time) -> std::size_t;
    auto once(std::function<bool()> func, std::size_t milliseconds) -> std::size_t;

  private:
    auto schedule(std::function<bool()> func, time_point time,
                  std::chrono::milliseconds interval = std::chrono::milliseconds{0}) -> std::size_t;
    void loop();

    ThreadPool& threadPool;

    std::priority_queue<Task> queue;
    std::mutex mutex;
    std::condition_variable cv;
    std::thread thread;
    std::atomic<bool> running{false};
    std::unordered_set<std::size_t> cancelled;
    std::atomic<std::size_t> next_id{1};
};

#endif // SCHEDULER_H