#include "core.h"

namespace MuonPi {

Core::Core()
{

}

Core::~Core()
{
    ThreadRunner::~ThreadRunner();
}

void Core::handle_event(std::unique_ptr<Event> /*event*/)
{

}

void Core::factor_changed(std::size_t /*hash*/, float /*factor*/)
{

}

void Core::detector_status_changed(std::size_t /*hash*/, Detector::Status /*status*/)
{

}

}
