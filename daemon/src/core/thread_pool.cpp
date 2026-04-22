#include "thread_pool.h"

#include "core/logging/logger.h"

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back(&ThreadPool::worker_loop, this);
    }
}

ThreadPool::~ThreadPool() {
    running = false;
    cv.notify_all();

    for (auto& t : workers) {
        if (t.joinable())
            t.join();
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(mutex);
        tasks.push(std::move(task));
    }
    cv.notify_one();
}

void ThreadPool::worker_loop() {
    while (running) {
        std::function<void()> task;

        {
            std::unique_lock lock(mutex);
            cv.wait(lock, [&] { return !tasks.empty() || !running; });

            if (!running && tasks.empty())
                return;

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
    logDebug("End worker loop");
}