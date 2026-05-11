#ifndef LOG_PARAMETER_PROCESSOR_H
#define LOG_PARAMETER_PROCESSOR_H

#include "core/event_bus.h"
#include "core/registries/data_store.h"
#include "data/events/ubx_event.h"
// #include "data/events/file_event.h"

class LogParameterProcessor {
  public:
    /**
     * Setup event triggered LogParameter events
     */
    static void setup(EventBus& bus);

    /**
     * Take data from datastore and publish LogParameter events
     */
    static void poll(EventBus& bus, const DataStore& datastore);
};

#endif // LOG_PARAMETER_PROCESSOR_H