#ifndef LOGGER_H
#define LOGGER_H

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

enum class LogLevel {
    Debug = 0,
    Info = 1,
    Warn = 2,
    Error = 3
};

class AsyncLogger
{
public:
    static auto instance() -> AsyncLogger&;

    AsyncLogger(const AsyncLogger&) = delete;
    auto operator=(const AsyncLogger&) -> AsyncLogger& = delete;

    void setMinimumLevel(LogLevel level);
    void log(LogLevel level, const std::string& message);
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

inline auto logLevel() { return AsyncLogger::instance().level(); }
inline void logDebug(const std::string& msg) { AsyncLogger::instance().log(LogLevel::Debug, msg); }
inline void logInfo(const std::string& msg) { AsyncLogger::instance().log(LogLevel::Info, msg); }
inline void logWarn(const std::string& msg) { AsyncLogger::instance().log(LogLevel::Warn, msg); }
inline void logError(const std::string& msg) { AsyncLogger::instance().log(LogLevel::Error, msg); }

#endif // LOGGER_H
