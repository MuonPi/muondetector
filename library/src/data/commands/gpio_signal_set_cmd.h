#ifndef GPIO_SIGNAL_SET_CMD_H
#define GPIO_SIGNAL_SET_CMD_H

#include "gpio_pin_definitions.h"

struct GpioSignalSetCmd {
    GPIO_SIGNAL sig;
    bool on;
};

#endif // GPIO_SIGNAL_SET_CMD_H