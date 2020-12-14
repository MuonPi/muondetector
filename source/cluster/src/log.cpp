#include "log.h"

namespace MuonPi::Log {


Sink::Sink(Level level)
    : m_level { level }
{}

Sink::~Sink() = default;

auto Sink::level() const -> Level
{
    return m_level;
}

auto Sink::to_string(Level level) -> std::string
{
    switch (level) {
    case Level::Debug:
        return "Debug";
    case Level::Info:
        return "Info";
    case Level::Notice:
        return "Notice";
    case Level::Warning:
        return "Warning";
    case Level::Error:
        return "Error";
    case Level::Critical:
        return "Critical";
    case Level::Alert:
        return "Alert";
    case Level::Emergency:
        return "Emergency";
    }
}

StreamSink::StreamSink(std::ostream& ostream, Level level)
    : Sink { level }
    , m_ostream { ostream }
{
}

void StreamSink::sink(const Message& msg)
{
    m_ostream<<to_string(msg.level) + ": " + msg.message + "\n"<<std::flush;
}
std::shared_ptr<Log> Log::s_singleton { std::make_shared<Log>() };

void Log::add_sink(std::shared_ptr<Sink> sink)
{
    m_sinks.push_back(sink);
}

SyslogSink::SyslogSink(Level level)
    : Sink { level }
{
    setlogmask(LOG_UPTO (level));
    openlog(appname, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}

SyslogSink::~SyslogSink()
{
    closelog();
}

void SyslogSink::sink(const Message& msg)
{
    syslog(level(), "%s", msg.message.c_str());
}

void Log::send(const Message& msg)
{
    for (auto& sink: m_sinks) {
        if (sink->level() >= msg.level) {
            sink->sink(msg);
        }
    }
}

auto Log::singleton() -> std::shared_ptr<Log>
{
    return s_singleton;
}


auto debug() -> Log::Logger<Level::Debug>&
{
    return Log::singleton()->m_debug;
}

auto info() -> Log::Logger<Level::Info>&
{
    return Log::singleton()->m_info;
}

auto notice() -> Log::Logger<Level::Notice>&
{
    return Log::singleton()->m_notice;
}

auto warning() -> Log::Logger<Level::Warning>&
{
    return Log::singleton()->m_warning;
}

auto error() -> Log::Logger<Level::Error>&
{
    return Log::singleton()->m_error;
}

auto critical() -> Log::Logger<Level::Critical>&
{
    return Log::singleton()->m_crititcal;
}

auto alert() -> Log::Logger<Level::Alert>&
{
    return Log::singleton()->m_alert;
}

auto emergency() -> Log::Logger<Level::Emergency>&
{
    return Log::singleton()->m_emergency;
}


}
