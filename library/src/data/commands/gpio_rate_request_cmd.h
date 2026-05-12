#ifndef GPIO_RATE_REQUEST_CMD_H
#define GPIO_RATE_REQUEST_CMD_H

#include <cstdint>

struct GpioRateRequestCmd {
    // 0 means "request all"
    std::uint8_t n_points{0};
};

#endif // GPIO_RATE_REQUEST_CMD_H
