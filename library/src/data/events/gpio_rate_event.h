#ifndef GPIO_RATE_EVENT_H
#define GPIO_RATE_EVENT_H

#include <cstdint>
#include <vector>

struct GpioRateEvent {
    std::uint8_t whichRate{0};
    std::vector<std::pair<float, float>> rate{};
};

#endif // GPIO_RATE_EVENT_H