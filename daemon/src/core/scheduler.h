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

class Scheduler {
  public:
    explicit Scheduler(ThreadPool& pool);
    ~Scheduler();

    void start();
    void stop();

    void every(std::chrono::milliseconds interval, std::function<void()> func);
    void once(std::function<void()> func, time_point time);
    void once(std::function<void()> func, std::size_t milliseconds);

  private:
    void schedule(std::function<void()> func, time_point time,
                  std::chrono::milliseconds interval = std::chrono::milliseconds{0});
    void loop();

    ThreadPool& threadPool;

    std::priority_queue<Task> queue;
    std::mutex mutex;
    std::condition_variable cv;
    std::thread thread;
    std::atomic<bool> running{false};
};

#endif // SCHEDULER_H