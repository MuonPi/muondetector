#include "thread_pool.h"

#include "core/logging/logger.h"

#include <pthread.h>

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this, i] {
            pthread_setname_np(pthread_self(), ("pool_" + std::to_string(i)).c_str());

            worker_loop();
        });
    }
}

ThreadPool::~ThreadPool() {
    stop();
}

void ThreadPool::stop() {
    {
        std::lock_guard lock(mutex);

        running = false;

        decltype(tasks) empty;
        tasks.swap(empty);
    }

    cv.notify_all();

    for (auto& t : workers) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(mutex);
        if (!running.load()) {
            return;
        }
        tasks.push(std::move(task));
    }
    cv.notify_one();
}

void ThreadPool::worker_loop() {
    for (;;) {
        std::function<void()> task;

        {
            std::unique_lock lock(mutex);

            cv.wait(lock, [&] { return !tasks.empty() || !running.load(); });

            if (!running.load() && tasks.empty()) {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }
        try {
            task();
        } catch (const std::exception& e) {
            logError(e.what()); // see if worker pool silently dies
        }
    }
    logDebug("End worker loop");
}