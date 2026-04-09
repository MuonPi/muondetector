#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ThreadPool {
public:
    explicit ThreadPool(size_t numThreads = std::max<std::size_t>(1, std::thread::hardware_concurrency()));
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