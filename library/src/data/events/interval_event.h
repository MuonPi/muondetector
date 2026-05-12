#ifndef INTERVAL_EVENT_H
#define INTERVAL_EVENT_H

#include "gpio_pin_definitions.h"

#include <chrono>

struct IntervalEvent {
    GPIO_SIGNAL sig;
    std::chrono::nanoseconds interval;
};

#endif // INTERVAL_EVENT_H