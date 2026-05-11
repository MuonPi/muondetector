#ifndef STATUS_LED_EVENT_H
#define STATUS_LED_EVENT_H

#include "gpio_pin_definitions.h"

#include <chrono>

struct StatusLedEvent {
    GPIO_SIGNAL sig{STATUS1};
    int durationMillisec{-1};
    bool on{true};
};

#endif // STATUS_LED_EVENT_H