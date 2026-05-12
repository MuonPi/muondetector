#ifndef GPIO_EVENT_H
#define GPIO_EVENT_H

#include "data/gpio_pin_definitions.h"

#include <chrono>

struct GpioEvent {
    GPIO_SIGNAL gpio_signal;
    unsigned int gpio_pin;
    std::chrono::steady_clock::time_point timestamp;
    EventEdge edge;
};

#endif // GPIO_EVENT_H