#ifndef LOGGER_H
#define LOGGER_H

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

enum class LogLevel { Debug = 0, Info = 1, Warn = 2, Error = 3 };

class AsyncLogger {
  public:
    static auto instance() -> AsyncLogger&;

    AsyncLogger(const AsyncLogger&) = delete;
    auto operator=(const AsyncLogger&) -> AsyncLogger& = delete;

    void setMinimumLevel(LogLevel level);
    void setMinimumLevel(const std::string& level);
    template <typename S>
    void log(LogLevel level, S&& message) {
        if (static_cast<int>(level) < static_cast<int>(minimumLevel_)) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);

            queue_.push(LogEntry{level, std::string(std::forward<S>(message))});
        }

        cv_.notify_one();
    }
    auto level() const -> LogLevel;

  private:
    AsyncLogger();
    ~AsyncLogger();

    void workerLoop();
    static auto levelName(LogLevel level) -> const char*;

    struct LogEntry {
        LogLevel level;
        std::string message;
    };

    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<LogEntry> queue_;
    std::thread worker_;
    bool running_{true};
    LogLevel minimumLevel_{LogLevel::Info};
};

inline auto logLevel() -> LogLevel {
    return AsyncLogger::instance().level();
}

inline void setLogLevel(const std::string& level) {
    AsyncLogger::instance().setMinimumLevel(level);
}
inline void setLogLevel(LogLevel level) {
    AsyncLogger::instance().setMinimumLevel(level);
}

template <typename S>
inline void logDebug(S&& msg) {
    AsyncLogger::instance().log(LogLevel::Debug, std::forward<S>(msg));
}
template <typename S>
inline void logInfo(S&& msg) {
    AsyncLogger::instance().log(LogLevel::Info, std::forward<S>(msg));
}
template <typename S>
inline void logWarn(S&& msg) {
    AsyncLogger::instance().log(LogLevel::Warn, std::forward<S>(msg));
}
template <typename S>
inline void logError(S&& msg) {
    AsyncLogger::instance().log(LogLevel::Error, std::forward<S>(msg));
}
#endif // LOGGER_H
