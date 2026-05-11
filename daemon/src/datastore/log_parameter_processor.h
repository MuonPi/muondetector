#ifndef LOG_PARAMETER_PROCESSOR_H
#define LOG_PARAMETER_PROCESSOR_H

#include "core/event_bus.h"
#include "data/events/ubx_event.h"
// #include "data/events/file_event.h"

class LogParameterProcessor {
  public:
    static void setup(EventBus& bus);
};

#endif // LOG_PARAMETER_PROCESSOR_H