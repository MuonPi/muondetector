#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
  public:
    explicit ThreadPool(
        size_t numThreads = std::max<std::size_t>(1, std::thread::hardware_concurrency()));
    ~ThreadPool();

    void enqueue(std::function<void()> task);

  private:
    void worker_loop();

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> running{true};
};

#endif // THREAD_POOL_H