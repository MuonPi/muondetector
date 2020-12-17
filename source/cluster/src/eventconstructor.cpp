#include "eventconstructor.h"
#include "criterion.h"
#include "log.h"

namespace MuonPi {

void EventConstructor::set_timeout(std::chrono::system_clock::duration new_timeout)
{
    if (new_timeout <= timeout) {
        return;
    }
    timeout = new_timeout;
}

auto EventConstructor::timed_out() const -> bool
{
    return (std::chrono::system_clock::now() - m_start) >= timeout;
}

}
