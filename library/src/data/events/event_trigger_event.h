#ifndef EVENT_TRIGGER_EVENT_H
#define EVENT_TRIGGER_EVENT_H

#include "data/gpio_pin_definitions.h"

struct EventTriggerEvent {
    GPIO_SIGNAL eventTrigger{EVT_XOR};
};

#endif // EVENT_TRIGGER_EVENT_H