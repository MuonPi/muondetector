#include "core.h"

#include "abstractsink.h"
#include "abstractsource.h"
#include "criterion.h"
#include "eventconstructor.h"
#include "event.h"
#include "logmessage.h"
#include "timebasesupervisor.h"

namespace MuonPi {

Core::Core()
{

}

Core::~Core()
{
    ThreadRunner::~ThreadRunner();
}

void Core::factor_changed(std::size_t /*hash*/, float /*factor*/)
{

}

void Core::detector_status_changed(std::size_t /*hash*/, Detector::Status /*status*/)
{

}

auto Core::step() -> bool
{
    return {};
}

void Core::handle_event(std::unique_ptr<Event> /*event*/)
{

}

void Core::handle_log(std::unique_ptr<LogMessage> /*log*/)
{

}

}
