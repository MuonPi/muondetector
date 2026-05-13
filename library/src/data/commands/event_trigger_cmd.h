#ifndef EVENT_TRIGGER_CMD_H
#define EVENT_TRIGGER_CMD_H

#include "data/gpio_pin_definitions.h"

struct EventTriggerCmd {
    GPIO_SIGNAL signal{EVT_XOR};
};

struct EventTriggerRequestCmd {};

#endif // EVENT_TRIGGER_CMD_H