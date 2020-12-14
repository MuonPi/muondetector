#ifndef LOG_H
#define LOG_H

#include <string>
#include <syslog.h>
#include <memory>
#include <vector>
#include <iostream>

namespace MuonPi::Log {

enum Level : int {
    Debug = LOG_DEBUG,
    Info = LOG_INFO,
    Notice = LOG_NOTICE,
    Warning = LOG_WARNING,
    Error = LOG_ERR,
    Critical = LOG_CRIT,
    Alert = LOG_ALERT,
    Emergency = LOG_EMERG
};

static constexpr const char* appname { "muondetector-cluster" };

struct Message
{
    const Level level { Level::Info };
    const std::string message {};
};


class Sink
{
public:
    Sink(Level level);

    virtual ~Sink();
    [[nodiscard]] auto level() const -> Level;

protected:
    friend class Log;

    [[nodiscard]] auto to_string(Level level) -> std::string;

    virtual void sink(const Message& msg) = 0;

private:
    Level m_level { Level::Info };
};

class StreamSink : public Sink
{
public:
    StreamSink(std::ostream& ostream, Level level = Level::Debug);

protected:
    void sink(const Message& msg) override;

private:
    std::ostream& m_ostream;
};

class SyslogSink : public Sink
{
public:
    SyslogSink(Level level = Level::Debug);

    ~SyslogSink();

protected:
    void sink(const Message& msg) override;
};


class Log
{
public:
    template <Level L>
    class Logger
    {
    public:
        Logger(Log& log);

        auto operator<<(std::string message) -> Logger<L>&;

    private:
        Log& m_log;
    };

    void add_sink(std::shared_ptr<Sink> sink);

    [[nodiscard]] static auto singleton() -> std::shared_ptr<Log>;

    Logger<Level::Debug> m_debug { *this };
    Logger<Level::Info> m_info { *this };
    Logger<Level::Notice> m_notice { *this };
    Logger<Level::Warning> m_warning { *this };
    Logger<Level::Error> m_error { *this };
    Logger<Level::Critical> m_crititcal { *this };
    Logger<Level::Alert> m_alert { *this };
    Logger<Level::Emergency> m_emergency { *this };

private:
    std::vector<std::shared_ptr<Sink>> m_sinks {};

    void send(const Message& msg);

    static std::shared_ptr<Log> s_singleton;
};

template <Level L>
Log::Logger<L>::Logger(Log& log)
    : m_log { log }
{}

template <Level L>
auto Log::Logger<L>::operator<<(std::string message) -> Logger<L>&
{
    m_log.send(Message{L, message});
    return *this;
}

[[nodiscard]] auto debug() -> Log::Logger<Level::Debug>&;
[[nodiscard]] auto info() -> Log::Logger<Level::Info>&;
[[nodiscard]] auto notice() -> Log::Logger<Level::Notice>&;
[[nodiscard]] auto warning() -> Log::Logger<Level::Warning>&;
[[nodiscard]] auto error() -> Log::Logger<Level::Error>&;
[[nodiscard]] auto critical() -> Log::Logger<Level::Critical>&;
[[nodiscard]] auto alert() -> Log::Logger<Level::Alert>&;
[[nodiscard]] auto emergency() -> Log::Logger<Level::Emergency>&;

}

#endif // LOG_H
