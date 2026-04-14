#include "core/logging/logger.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

auto AsyncLogger::instance() -> AsyncLogger&
{
    static AsyncLogger logger;
    return logger;
}

AsyncLogger::AsyncLogger()
{
    worker_ = std::thread(&AsyncLogger::workerLoop, this);
}

AsyncLogger::~AsyncLogger()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    cv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void AsyncLogger::setMinimumLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(mutex_);
    minimumLevel_ = level;
}

void AsyncLogger::log(LogLevel level, const std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (static_cast<int>(level) < static_cast<int>(minimumLevel_)) {
        return;
    }
    queue_.push(LogEntry{ level, message });
    cv_.notify_one();
}


auto AsyncLogger::level() const -> LogLevel
{
    return minimumLevel_;
}

auto AsyncLogger::levelName(LogLevel level) -> const char*
{
    switch (level) {
    case LogLevel::Debug:
        return "DEBUG";
    case LogLevel::Info:
        return "INFO";
    case LogLevel::Warn:
        return "WARN";
    case LogLevel::Error:
        return "ERROR";
    default:
        return "LOG";
    }
}

void AsyncLogger::workerLoop()
{
    for (;;) {
        LogEntry entry;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() {
                return !running_ || !queue_.empty();
            });

            if (!running_ && queue_.empty()) {
                return;
            }

            entry = std::move(queue_.front());
            queue_.pop();
        }

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm local{};
#if defined(_WIN32)
        localtime_s(&local, &t);
#else
        localtime_r(&t, &local);
#endif

        std::ostringstream line;
        line << std::put_time(&local, "%Y-%m-%d %H:%M:%S")
             << " [" << levelName(entry.level) << "] "
             << entry.message;

        if (entry.level == LogLevel::Warn || entry.level == LogLevel::Error) {
            std::cerr << line.str() << '\n';
            continue;
        }
        std::clog << line.str() << '\n';
    }
}
