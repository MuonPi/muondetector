#ifndef AD1115_EVENT_H
#define AD1115_EVENT_H

#include <cstdint>

struct Ad1115SampleEvent
{
    std::uint32_t deviceId;
    std::uint8_t channel;
    std::uint16_t rawValue;
    float voltage;
    std::uint64_t timestamp;
};

#endif // AD1115_EVENT_H
