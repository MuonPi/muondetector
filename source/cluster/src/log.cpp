#include "log.h"

namespace MuonPi::Log {

void init(Level level)
{
    setlogmask(LOG_UPTO (level));
    openlog(appname, LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}

void finish()
{
    closelog();
}

}
