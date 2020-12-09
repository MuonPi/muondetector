#ifndef LOG_H
#define LOG_H

#include <string>
#include <syslog.h>

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

/**
 * @brief init Initialises the logging system
 * @param level The minimum log level to show. Inclusive.
 */
void init(Level level);

void finish();

}

#endif // LOG_H
