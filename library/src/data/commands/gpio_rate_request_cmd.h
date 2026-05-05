#ifndef GPIO_RATE_REQUEST_CMD_H
#define GPIO_RATE_REQUEST_CMD_H

#include <cstdint>

struct GpioRateRequestCmd {
    std::uint8_t whichRate{0};
};

#endif // GPIO_RATE_REQUEST_CMD_H
